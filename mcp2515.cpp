#include "Arduino.h"
#include "mcp2515.h"

const struct MCP2515::TXBn_REGS MCP2515::TXB[MCP2515::N_TXBUFFERS] = {
    {MCP_TXB0CTRL, MCP_TXB0SIDH, MCP_TXB0DATA},
    {MCP_TXB1CTRL, MCP_TXB1SIDH, MCP_TXB1DATA},
    {MCP_TXB2CTRL, MCP_TXB2SIDH, MCP_TXB2DATA}
};

const struct MCP2515::RXBn_REGS MCP2515::RXB[N_RXBUFFERS] = {
    {MCP_RXB0CTRL, MCP_RXB0SIDH, MCP_RXB0DATA, CANINTF_RX0IF},
    {MCP_RXB1CTRL, MCP_RXB1SIDH, MCP_RXB1DATA, CANINTF_RX1IF}
};

MCP2515::MCP2515(const uint8_t _CS, const uint32_t _SPI_CLOCK, SPIClass * _SPI)
{
    if (_SPI != nullptr) {
        SPIn = _SPI;
    }
    else {
        SPIn = &SPI;
        SPIn->begin();
    }

    SPICS = _CS;
    SPI_CLOCK = _SPI_CLOCK;
    pinMode(SPICS, OUTPUT);
    digitalWrite(SPICS, HIGH);
}

void MCP2515::startSPI() {
    SPIn->beginTransaction(SPISettings(SPI_CLOCK, MSBFIRST, SPI_MODE0));
    digitalWrite(SPICS, LOW);
}

void MCP2515::endSPI() {
    digitalWrite(SPICS, HIGH);
    SPIn->endTransaction();
}

void MCP2515::transferSPI(uint8_t *buffer, const uint8_t n) {
#if defined(ARDUINO_ARCH_ESP32)
    // Use the hardware FIFO within the existing, locked SPI transaction.
    // Padding is storage only: exactly n bytes are clocked on the bus.
    SPIn->transferBytes(buffer, buffer, n);
#else
    for (uint8_t i = 0; i < n; i++) {
        buffer[i] = SPIn->transfer(buffer[i]);
    }
#endif
}

MCP2515::ERROR MCP2515::reset(void)
{
    preferRx1 = false;
    startSPI();
    SPIn->transfer(INSTRUCTION_RESET);
    endSPI();

    delay(10);

    uint8_t zeros[14];
    memset(zeros, 0, sizeof(zeros));
    setRegisters(MCP_TXB0CTRL, zeros, 14);
    setRegisters(MCP_TXB1CTRL, zeros, 14);
    setRegisters(MCP_TXB2CTRL, zeros, 14);

    setRegister(MCP_RXB0CTRL, 0);
    setRegister(MCP_RXB1CTRL, 0);

    setRegister(MCP_CANINTE, CANINTF_RX0IF | CANINTF_RX1IF | CANINTF_ERRIF | CANINTF_MERRF);

    // Accept valid standard/extended frames matching the filters. RXB0 uses
    // RXF0/RXF1; RXB1 uses RXF2..RXF5. BUKT lets RXB0 spill into RXB1.
    modifyRegister(MCP_RXB0CTRL,
                   RXBnCTRL_RXM_MASK | RXB0CTRL_BUKT | RXB0CTRL_FILHIT_MASK,
                   RXBnCTRL_RXM_STDEXT | RXB0CTRL_BUKT | RXB0CTRL_FILHIT);
    modifyRegister(MCP_RXB1CTRL,
                   RXBnCTRL_RXM_MASK | RXB1CTRL_FILHIT_MASK,
                   RXBnCTRL_RXM_STDEXT | RXB1CTRL_FILHIT);

    // clear filters and masks
    // RXF1 accepts extended frames on RXB0; the other filters accept standard
    // frames. Zero masks accept every identifier of the matching frame type.
    RXF filters[] = {RXF0, RXF1, RXF2, RXF3, RXF4, RXF5};
    for (int i=0; i<6; i++) {
        bool ext = (i == 1);
        ERROR result = setFilter(filters[i], ext, 0);
        if (result != ERROR_OK) {
            return result;
        }
    }

    MASK masks[] = {MASK0, MASK1};
    for (int i=0; i<2; i++) {
        ERROR result = setFilterMask(masks[i], true, 0);
        if (result != ERROR_OK) {
            return result;
        }
    }

    uint8_t rx0Ctrl, rx1Ctrl;
    getRxBufferConfiguration(rx0Ctrl, rx1Ctrl);
    if ((rx0Ctrl & (RXBnCTRL_RXM_MASK | RXB0CTRL_BUKT)) != RXB0CTRL_BUKT ||
        (rx1Ctrl & RXBnCTRL_RXM_MASK) != RXBnCTRL_RXM_STDEXT) {
        return ERROR_FAIL;
    }
    return ERROR_OK;
}

uint8_t MCP2515::readRegister(const REGISTER reg)
{
    alignas(4) uint8_t buffer[4] = {INSTRUCTION_READ, static_cast<uint8_t>(reg), 0, 0};
    startSPI();
    transferSPI(buffer, 3);
    endSPI();

    return buffer[2];
}

void MCP2515::readRegisters(const REGISTER reg, uint8_t values[], const uint8_t n)
{
    alignas(4) uint8_t buffer[16] = {INSTRUCTION_READ, static_cast<uint8_t>(reg)};
    uint8_t offset = 0;
    uint8_t prefix = 2;
    startSPI();
    // MCP2515 auto-increments the address while CS remains low.
    do {
        const uint8_t remaining = n - offset;
        const uint8_t capacity = sizeof(buffer) - prefix;
        const uint8_t count = remaining < capacity ? remaining : capacity;
        transferSPI(buffer, prefix + count);
        if (count) memcpy(values + offset, buffer + prefix, count);
        offset += count;
        prefix = 0;
        memset(buffer, 0, sizeof(buffer));
    } while (offset < n);
    endSPI();
}

void MCP2515::setRegister(const REGISTER reg, const uint8_t value)
{
    alignas(4) uint8_t buffer[4] = {INSTRUCTION_WRITE, static_cast<uint8_t>(reg), value, 0};
    startSPI();
    transferSPI(buffer, 3);
    endSPI();
}

void MCP2515::setRegisters(const REGISTER reg, const uint8_t values[], const uint8_t n)
{
    alignas(4) uint8_t buffer[16] = {INSTRUCTION_WRITE, static_cast<uint8_t>(reg)};
    uint8_t offset = 0;
    uint8_t prefix = 2;
    startSPI();
    do {
        const uint8_t remaining = n - offset;
        const uint8_t capacity = sizeof(buffer) - prefix;
        const uint8_t count = remaining < capacity ? remaining : capacity;
        if (count) memcpy(buffer + prefix, values + offset, count);
        transferSPI(buffer, prefix + count);
        offset += count;
        prefix = 0;
    } while (offset < n);
    endSPI();
}

void MCP2515::modifyRegister(const REGISTER reg, const uint8_t mask, const uint8_t data)
{
    alignas(4) uint8_t buffer[4] = {INSTRUCTION_BITMOD, static_cast<uint8_t>(reg), mask, data};
    startSPI();
    transferSPI(buffer, sizeof(buffer));
    endSPI();
}

uint8_t MCP2515::getStatus(void)
{
    alignas(4) uint8_t buffer[4] = {INSTRUCTION_READ_STATUS, 0, 0, 0};
    startSPI();
    transferSPI(buffer, 2);
    endSPI();

    return buffer[1];
}

MCP2515::ERROR MCP2515::setConfigMode()
{
    return setMode(CANCTRL_REQOP_CONFIG);
}

MCP2515::ERROR MCP2515::setListenOnlyMode()
{
    return setMode(CANCTRL_REQOP_LISTENONLY);
}

MCP2515::ERROR MCP2515::setSleepMode()
{
    return setMode(CANCTRL_REQOP_SLEEP);
}

MCP2515::ERROR MCP2515::setLoopbackMode()
{
    return setMode(CANCTRL_REQOP_LOOPBACK);
}

MCP2515::ERROR MCP2515::setNormalMode()
{
    return setMode(CANCTRL_REQOP_NORMAL);
}

MCP2515::ERROR MCP2515::setNormalOneShotMode()
{
    return setMode(CANCTRL_REQOP_OSM);
}

MCP2515::ERROR MCP2515::setMode(const CANCTRL_REQOP_MODE mode)
{
    modifyRegister(MCP_CANCTRL, CANCTRL_REQOP | CANCTRL_OSM, mode);

    const unsigned long startTime = millis();
    bool modeMatch = false;
    while (millis() - startTime < 10) {
        uint8_t newmode = readRegister(MCP_CANSTAT);
        newmode &= CANSTAT_OPMOD;

        // OSM is a CANCTRL option, not part of CANSTAT's operation mode.
        modeMatch = newmode == (mode & CANCTRL_REQOP);
        if (modeMatch) {
            modeMatch = (readRegister(MCP_CANCTRL) & CANCTRL_OSM)
                        == (mode & CANCTRL_OSM);
        }

        if (modeMatch) {
            break;
        }
    }

    return modeMatch ? ERROR_OK : ERROR_FAIL;

}

MCP2515::ERROR MCP2515::setBitrate(const CAN_SPEED canSpeed)
{
    return setBitrate(canSpeed, MCP_16MHZ);
}

MCP2515::ERROR MCP2515::setBitrate(const CAN_SPEED canSpeed, CAN_CLOCK canClock)
{
    ERROR error = setConfigMode();
    if (error != ERROR_OK) {
        return error;
    }

    uint8_t set, cfg1, cfg2, cfg3;
    set = 1;
    switch (canClock)
    {
        case (MCP_8MHZ):
        switch (canSpeed)
        {
            case (CAN_5KBPS):                                               //   5KBPS
            cfg1 = MCP_8MHz_5kBPS_CFG1;
            cfg2 = MCP_8MHz_5kBPS_CFG2;
            cfg3 = MCP_8MHz_5kBPS_CFG3;
            break;

            case (CAN_10KBPS):                                              //  10KBPS
            cfg1 = MCP_8MHz_10kBPS_CFG1;
            cfg2 = MCP_8MHz_10kBPS_CFG2;
            cfg3 = MCP_8MHz_10kBPS_CFG3;
            break;

            case (CAN_20KBPS):                                              //  20KBPS
            cfg1 = MCP_8MHz_20kBPS_CFG1;
            cfg2 = MCP_8MHz_20kBPS_CFG2;
            cfg3 = MCP_8MHz_20kBPS_CFG3;
            break;

            case (CAN_31K25BPS):                                            //  31.25KBPS
            cfg1 = MCP_8MHz_31k25BPS_CFG1;
            cfg2 = MCP_8MHz_31k25BPS_CFG2;
            cfg3 = MCP_8MHz_31k25BPS_CFG3;
            break;

            case (CAN_33KBPS):                                              //  33.333KBPS
            cfg1 = MCP_8MHz_33k3BPS_CFG1;
            cfg2 = MCP_8MHz_33k3BPS_CFG2;
            cfg3 = MCP_8MHz_33k3BPS_CFG3;
            break;

            case (CAN_40KBPS):                                              //  40Kbps
            cfg1 = MCP_8MHz_40kBPS_CFG1;
            cfg2 = MCP_8MHz_40kBPS_CFG2;
            cfg3 = MCP_8MHz_40kBPS_CFG3;
            break;

            case (CAN_50KBPS):                                              //  50Kbps
            cfg1 = MCP_8MHz_50kBPS_CFG1;
            cfg2 = MCP_8MHz_50kBPS_CFG2;
            cfg3 = MCP_8MHz_50kBPS_CFG3;
            break;

            case (CAN_80KBPS):                                              //  80Kbps
            cfg1 = MCP_8MHz_80kBPS_CFG1;
            cfg2 = MCP_8MHz_80kBPS_CFG2;
            cfg3 = MCP_8MHz_80kBPS_CFG3;
            break;

            case (CAN_100KBPS):                                             // 100Kbps
            cfg1 = MCP_8MHz_100kBPS_CFG1;
            cfg2 = MCP_8MHz_100kBPS_CFG2;
            cfg3 = MCP_8MHz_100kBPS_CFG3;
            break;

            case (CAN_125KBPS):                                             // 125Kbps
            cfg1 = MCP_8MHz_125kBPS_CFG1;
            cfg2 = MCP_8MHz_125kBPS_CFG2;
            cfg3 = MCP_8MHz_125kBPS_CFG3;
            break;

            case (CAN_200KBPS):                                             // 200Kbps
            cfg1 = MCP_8MHz_200kBPS_CFG1;
            cfg2 = MCP_8MHz_200kBPS_CFG2;
            cfg3 = MCP_8MHz_200kBPS_CFG3;
            break;

            case (CAN_250KBPS):                                             // 250Kbps
            cfg1 = MCP_8MHz_250kBPS_CFG1;
            cfg2 = MCP_8MHz_250kBPS_CFG2;
            cfg3 = MCP_8MHz_250kBPS_CFG3;
            break;

            case (CAN_500KBPS):                                             // 500Kbps
            cfg1 = MCP_8MHz_500kBPS_CFG1;
            cfg2 = MCP_8MHz_500kBPS_CFG2;
            cfg3 = MCP_8MHz_500kBPS_CFG3;
            break;

            case (CAN_1000KBPS):                                            //   1Mbps
            cfg1 = MCP_8MHz_1000kBPS_CFG1;
            cfg2 = MCP_8MHz_1000kBPS_CFG2;
            cfg3 = MCP_8MHz_1000kBPS_CFG3;
            break;

            default:
            set = 0;
            break;
        }
        break;

        case (MCP_16MHZ):
        switch (canSpeed)
        {
            case (CAN_5KBPS):                                               //   5Kbps
            cfg1 = MCP_16MHz_5kBPS_CFG1;
            cfg2 = MCP_16MHz_5kBPS_CFG2;
            cfg3 = MCP_16MHz_5kBPS_CFG3;
            break;

            case (CAN_10KBPS):                                              //  10Kbps
            cfg1 = MCP_16MHz_10kBPS_CFG1;
            cfg2 = MCP_16MHz_10kBPS_CFG2;
            cfg3 = MCP_16MHz_10kBPS_CFG3;
            break;

            case (CAN_20KBPS):                                              //  20Kbps
            cfg1 = MCP_16MHz_20kBPS_CFG1;
            cfg2 = MCP_16MHz_20kBPS_CFG2;
            cfg3 = MCP_16MHz_20kBPS_CFG3;
            break;

            case (CAN_33KBPS):                                              //  33.333Kbps
            cfg1 = MCP_16MHz_33k3BPS_CFG1;
            cfg2 = MCP_16MHz_33k3BPS_CFG2;
            cfg3 = MCP_16MHz_33k3BPS_CFG3;
            break;

            case (CAN_40KBPS):                                              //  40Kbps
            cfg1 = MCP_16MHz_40kBPS_CFG1;
            cfg2 = MCP_16MHz_40kBPS_CFG2;
            cfg3 = MCP_16MHz_40kBPS_CFG3;
            break;

            case (CAN_50KBPS):                                              //  50Kbps
            cfg1 = MCP_16MHz_50kBPS_CFG1;
            cfg2 = MCP_16MHz_50kBPS_CFG2;
            cfg3 = MCP_16MHz_50kBPS_CFG3;
            break;

            case (CAN_80KBPS):                                              //  80Kbps
            cfg1 = MCP_16MHz_80kBPS_CFG1;
            cfg2 = MCP_16MHz_80kBPS_CFG2;
            cfg3 = MCP_16MHz_80kBPS_CFG3;
            break;

            case (CAN_83K3BPS):                                             //  83.333Kbps
            cfg1 = MCP_16MHz_83k3BPS_CFG1;
            cfg2 = MCP_16MHz_83k3BPS_CFG2;
            cfg3 = MCP_16MHz_83k3BPS_CFG3;
            break;

            case (CAN_95KBPS):                                              //  95Kbps
            cfg1 = MCP_16MHz_95kBPS_CFG1;
            cfg2 = MCP_16MHz_95kBPS_CFG2;
            cfg3 = MCP_16MHz_95kBPS_CFG3;
            break;

            case (CAN_100KBPS):                                             // 100Kbps
            cfg1 = MCP_16MHz_100kBPS_CFG1;
            cfg2 = MCP_16MHz_100kBPS_CFG2;
            cfg3 = MCP_16MHz_100kBPS_CFG3;
            break;

            case (CAN_125KBPS):                                             // 125Kbps
            cfg1 = MCP_16MHz_125kBPS_CFG1;
            cfg2 = MCP_16MHz_125kBPS_CFG2;
            cfg3 = MCP_16MHz_125kBPS_CFG3;
            break;

            case (CAN_200KBPS):                                             // 200Kbps
            cfg1 = MCP_16MHz_200kBPS_CFG1;
            cfg2 = MCP_16MHz_200kBPS_CFG2;
            cfg3 = MCP_16MHz_200kBPS_CFG3;
            break;

            case (CAN_250KBPS):                                             // 250Kbps
            cfg1 = MCP_16MHz_250kBPS_CFG1;
            cfg2 = MCP_16MHz_250kBPS_CFG2;
            cfg3 = MCP_16MHz_250kBPS_CFG3;
            break;

            case (CAN_500KBPS):                                             // 500Kbps
            cfg1 = MCP_16MHz_500kBPS_CFG1;
            cfg2 = MCP_16MHz_500kBPS_CFG2;
            cfg3 = MCP_16MHz_500kBPS_CFG3;
            break;

            case (CAN_1000KBPS):                                            //   1Mbps
            cfg1 = MCP_16MHz_1000kBPS_CFG1;
            cfg2 = MCP_16MHz_1000kBPS_CFG2;
            cfg3 = MCP_16MHz_1000kBPS_CFG3;
            break;

            default:
            set = 0;
            break;
        }
        break;

        case (MCP_20MHZ):
        switch (canSpeed)
        {
            case (CAN_33KBPS):                                              //  33.333Kbps
            cfg1 = MCP_20MHz_33k3BPS_CFG1;
            cfg2 = MCP_20MHz_33k3BPS_CFG2;
            cfg3 = MCP_20MHz_33k3BPS_CFG3;
	    break;

            case (CAN_40KBPS):                                              //  40Kbps
            cfg1 = MCP_20MHz_40kBPS_CFG1;
            cfg2 = MCP_20MHz_40kBPS_CFG2;
            cfg3 = MCP_20MHz_40kBPS_CFG3;
            break;

            case (CAN_50KBPS):                                              //  50Kbps
            cfg1 = MCP_20MHz_50kBPS_CFG1;
            cfg2 = MCP_20MHz_50kBPS_CFG2;
            cfg3 = MCP_20MHz_50kBPS_CFG3;
            break;

            case (CAN_80KBPS):                                              //  80Kbps
            cfg1 = MCP_20MHz_80kBPS_CFG1;
            cfg2 = MCP_20MHz_80kBPS_CFG2;
            cfg3 = MCP_20MHz_80kBPS_CFG3;
            break;

            case (CAN_83K3BPS):                                             //  83.333Kbps
            cfg1 = MCP_20MHz_83k3BPS_CFG1;
            cfg2 = MCP_20MHz_83k3BPS_CFG2;
            cfg3 = MCP_20MHz_83k3BPS_CFG3;
	    break;

            case (CAN_100KBPS):                                             // 100Kbps
            cfg1 = MCP_20MHz_100kBPS_CFG1;
            cfg2 = MCP_20MHz_100kBPS_CFG2;
            cfg3 = MCP_20MHz_100kBPS_CFG3;
            break;

            case (CAN_125KBPS):                                             // 125Kbps
            cfg1 = MCP_20MHz_125kBPS_CFG1;
            cfg2 = MCP_20MHz_125kBPS_CFG2;
            cfg3 = MCP_20MHz_125kBPS_CFG3;
            break;

            case (CAN_200KBPS):                                             // 200Kbps
            cfg1 = MCP_20MHz_200kBPS_CFG1;
            cfg2 = MCP_20MHz_200kBPS_CFG2;
            cfg3 = MCP_20MHz_200kBPS_CFG3;
            break;

            case (CAN_250KBPS):                                             // 250Kbps
            cfg1 = MCP_20MHz_250kBPS_CFG1;
            cfg2 = MCP_20MHz_250kBPS_CFG2;
            cfg3 = MCP_20MHz_250kBPS_CFG3;
            break;

            case (CAN_500KBPS):                                             // 500Kbps
            cfg1 = MCP_20MHz_500kBPS_CFG1;
            cfg2 = MCP_20MHz_500kBPS_CFG2;
            cfg3 = MCP_20MHz_500kBPS_CFG3;
            break;

            case (CAN_1000KBPS):                                            //   1Mbps
            cfg1 = MCP_20MHz_1000kBPS_CFG1;
            cfg2 = MCP_20MHz_1000kBPS_CFG2;
            cfg3 = MCP_20MHz_1000kBPS_CFG3;
            break;

            default:
            set = 0;
            break;
        }
        break;

        default:
        set = 0;
        break;
    }

    if (set) {
        setRegister(MCP_CNF1, cfg1);
        setRegister(MCP_CNF2, cfg2);
        setRegister(MCP_CNF3, cfg3);
        return ERROR_OK;
    }
    else {
        return ERROR_FAIL;
    }
}

MCP2515::ERROR MCP2515::setClkOut(const CAN_CLKOUT divisor)
{
    if (divisor == CLKOUT_DISABLE) {
	/* Turn off CLKEN */
	modifyRegister(MCP_CANCTRL, CANCTRL_CLKEN, 0x00);

	/* Turn on CLKOUT for SOF */
	modifyRegister(MCP_CNF3, CNF3_SOF, CNF3_SOF);
        return ERROR_OK;
    }

    /* Set the prescaler (CLKPRE) */
    modifyRegister(MCP_CANCTRL, CANCTRL_CLKPRE, divisor);

    /* Turn on CLKEN */
    modifyRegister(MCP_CANCTRL, CANCTRL_CLKEN, CANCTRL_CLKEN);

    /* Turn off CLKOUT for SOF */
    modifyRegister(MCP_CNF3, CNF3_SOF, 0x00);
    return ERROR_OK;
}

void MCP2515::prepareId(uint8_t *buffer, const bool ext, const uint32_t id)
{
    uint16_t canid = (uint16_t)(id & 0x0FFFF);

    if (ext) {
        buffer[MCP_EID0] = (uint8_t) (canid & 0xFF);
        buffer[MCP_EID8] = (uint8_t) (canid >> 8);
        canid = (uint16_t)(id >> 16);
        buffer[MCP_SIDL] = (uint8_t) (canid & 0x03);
        buffer[MCP_SIDL] += (uint8_t) ((canid & 0x1C) << 3);
        buffer[MCP_SIDL] |= TXB_EXIDE_MASK;
        buffer[MCP_SIDH] = (uint8_t) (canid >> 5);
    } else {
        buffer[MCP_SIDH] = (uint8_t) (canid >> 3);
        buffer[MCP_SIDL] = (uint8_t) ((canid & 0x07 ) << 5);
        buffer[MCP_EID0] = 0;
        buffer[MCP_EID8] = 0;
    }
}

MCP2515::ERROR MCP2515::setFilterMask(const MASK mask, const bool ext, const uint32_t ulData)
{
    ERROR res = setConfigMode();
    if (res != ERROR_OK) {
        return res;
    }
    
    uint8_t tbufdata[4];
    prepareId(tbufdata, ext, ulData);

    REGISTER reg;
    switch (mask) {
        case MASK0: reg = MCP_RXM0SIDH; break;
        case MASK1: reg = MCP_RXM1SIDH; break;
        default:
            return ERROR_FAIL;
    }

    setRegisters(reg, tbufdata, 4);
    
    return ERROR_OK;
}

MCP2515::ERROR MCP2515::setFilter(const RXF num, const bool ext, const uint32_t ulData)
{
    ERROR res = setConfigMode();
    if (res != ERROR_OK) {
        return res;
    }

    REGISTER reg;

    switch (num) {
        case RXF0: reg = MCP_RXF0SIDH; break;
        case RXF1: reg = MCP_RXF1SIDH; break;
        case RXF2: reg = MCP_RXF2SIDH; break;
        case RXF3: reg = MCP_RXF3SIDH; break;
        case RXF4: reg = MCP_RXF4SIDH; break;
        case RXF5: reg = MCP_RXF5SIDH; break;
        default:
            return ERROR_FAIL;
    }

    uint8_t tbufdata[4];
    prepareId(tbufdata, ext, ulData);
    setRegisters(reg, tbufdata, 4);

    return ERROR_OK;
}

MCP2515::ERROR MCP2515::sendMessage(const TXBn txbn, const struct can_frame *frame)
{
    if (frame->can_dlc > CAN_MAX_DLEN) {
        return ERROR_FAILTX;
    }

    const struct TXBn_REGS *txbuf = &TXB[txbn];

    if (readRegister(txbuf->CTRL) & TXB_TXREQ) {
        return ERROR_ALLTXBUSY;
    }

    return sendMessageToBuffer(txbn, frame);
}

MCP2515::ERROR MCP2515::sendMessageToBuffer(const TXBn txbn, const struct can_frame *frame)
{
    const struct TXBn_REGS *txbuf = &TXB[txbn];
    alignas(4) uint8_t buffer[16] = {};
    uint8_t *header = buffer + 1;
    buffer[0] = INSTRUCTION_LOAD_TX0 + 2 * txbn;

    bool ext = (frame->can_id & CAN_EFF_FLAG);
    bool rtr = (frame->can_id & CAN_RTR_FLAG);
    uint32_t id = (frame->can_id & (ext ? CAN_EFF_MASK : CAN_SFF_MASK));

    prepareId(header, ext, id);

    header[MCP_DLC] = rtr ? (frame->can_dlc | RTR_MASK) : frame->can_dlc;

    // Remote frames carry a requested DLC but no data payload.
    const uint8_t payloadLength = rtr ? 0 : frame->can_dlc;
    if (payloadLength) memcpy(buffer + 6, frame->data, payloadLength);
    startSPI();
    transferSPI(buffer, 6 + payloadLength);
    endSPI();

    startSPI();
    SPIn->transfer(0x80 | (1 << txbn));
    endSPI();

    uint8_t ctrl = readRegister(txbuf->CTRL);
    if ((ctrl & (TXB_ABTF | TXB_MLOA | TXB_TXERR)) != 0) {
        return ERROR_FAILTX;
    }
    return ERROR_OK;
}

MCP2515::ERROR MCP2515::sendMessage(const struct can_frame *frame)
{
    if (frame->can_dlc > CAN_MAX_DLEN) {
        return ERROR_FAILTX;
    }

    // READ STATUS reports all three TXREQ bits at positions 2, 4 and 6.
    const uint8_t status = getStatus();
    uint8_t busyMask = 0x04;
    for (int i = 0; i < N_TXBUFFERS; i++, busyMask <<= 2) {
        if ((status & busyMask) == 0) {
            return sendMessageToBuffer(static_cast<TXBn>(i), frame);
        }
    }

    return ERROR_ALLTXBUSY;
}

MCP2515::ERROR MCP2515::readMessage(const RXBn rxbn, struct can_frame *frame)
{
    alignas(4) uint8_t buffer[8] = {};
    buffer[0] = rxbn == RXB0 ? INSTRUCTION_READ_RX0 : INSTRUCTION_READ_RX1;
    const uint8_t *tbufdata = buffer + 1;

    // Keep CS low through the header and payload. Raising CS clears only RXnIF.
    startSPI();
    transferSPI(buffer, 6);

    uint8_t dlc = (tbufdata[MCP_DLC] & DLC_MASK);
    if (dlc > CAN_MAX_DLEN) {
        endSPI();
        return ERROR_FAIL;
    }
    if (dlc) {
        alignas(4) uint8_t payload[8] = {};
        transferSPI(payload, dlc);
        memcpy(frame->data, payload, dlc);
    }
    endSPI();

    uint32_t id = (tbufdata[MCP_SIDH]<<3) + (tbufdata[MCP_SIDL]>>5);

    const bool ext = (tbufdata[MCP_SIDL] & TXB_EXIDE_MASK) != 0;
    if (ext) {
        id = (id<<2) + (tbufdata[MCP_SIDL] & 0x03);
        id = (id<<8) + tbufdata[MCP_EID8];
        id = (id<<8) + tbufdata[MCP_EID0];
        id |= CAN_EFF_FLAG;
    }

    // Standard RTR is in SIDL.SRR; extended RTR is in DLC.RTR.
    if (ext ? (tbufdata[MCP_DLC] & RTR_MASK) : (tbufdata[MCP_SIDL] & RXBnSIDL_SRR)) {
        id |= CAN_RTR_FLAG;
    }

    frame->can_id = id;
    frame->can_dlc = dlc;

    return ERROR_OK;
}

MCP2515::ERROR MCP2515::readMessage(struct can_frame *frame)
{
    return readMessage(frame, nullptr);
}

MCP2515::ERROR MCP2515::readMessage(struct can_frame *frame, RXBn *bufferRead)
{
    return readMessage(frame, bufferRead, nullptr);
}

MCP2515::ERROR MCP2515::readMessage(struct can_frame *frame, RXBn *bufferRead,
                                  uint8_t *statusRead)
{
    const uint8_t stat = getStatus();
    // READ STATUS includes TXREQ as well as RX flags. Expose this observation
    // for diagnostics without issuing another SPI transaction.
    if (statusRead) *statusRead = stat;
    if (!(stat & STAT_RXIF_MASK)) return ERROR_NOMSG;
    // When RX0 refills continuously, fixed RX0 priority starves RX1. Rotate
    // priority per controller, while always reading the sole pending buffer.
    const RXBn selected = (stat & STAT_RX1IF) &&
        (!(stat & STAT_RX0IF) || preferRx1) ? RXB1 : RXB0;
    const ERROR result = readMessage(selected, frame);
    preferRx1 = selected == RXB0;
    if (result == ERROR_OK && bufferRead) *bufferRead = selected;
    return result;
}

void MCP2515::getRxBufferConfiguration(uint8_t &rx0Ctrl, uint8_t &rx1Ctrl)
{
    rx0Ctrl = readRegister(MCP_RXB0CTRL);
    rx1Ctrl = readRegister(MCP_RXB1CTRL);
}

bool MCP2515::checkReceive(void)
{
    uint8_t res = getStatus();
    if ( res & STAT_RXIF_MASK ) {
        return true;
    } else {
        return false;
    }
}

bool MCP2515::checkError(void)
{
    uint8_t eflg = getErrorFlags();

    if ( eflg & EFLG_ERRORMASK ) {
        return true;
    } else {
        return false;
    }
}

uint8_t MCP2515::getErrorFlags(void)
{
    return readRegister(MCP_EFLG);
}

uint8_t MCP2515::getControlRegister(void) 
{
    return readRegister(MCP_CANCTRL);
}

void MCP2515::clearRXnOVRFlags(void)
{
	modifyRegister(MCP_EFLG, EFLG_RX0OVR | EFLG_RX1OVR, 0);
}

uint8_t MCP2515::getInterrupts(void)
{
    return readRegister(MCP_CANINTF);
}

void MCP2515::clearInterrupts(void)
{
    setRegister(MCP_CANINTF, 0);
}

uint8_t MCP2515::getInterruptMask(void)
{
    return readRegister(MCP_CANINTE);
}

void MCP2515::clearTXInterrupts(void)
{
    modifyRegister(MCP_CANINTF, (CANINTF_TX0IF | CANINTF_TX1IF | CANINTF_TX2IF), 0);
}

void MCP2515::clearRXnOVR(void)
{
	uint8_t eflg = getErrorFlags();
	if (eflg & (EFLG_RX0OVR | EFLG_RX1OVR)) {
		clearRXnOVRFlags();
		clearERRIF();
	}
	
}

void MCP2515::clearMERR()
{
	//modifyRegister(MCP_EFLG, EFLG_RX0OVR | EFLG_RX1OVR, 0);
	//clearInterrupts();
	modifyRegister(MCP_CANINTF, CANINTF_MERRF, 0);
}

void MCP2515::clearERRIF()
{
    //modifyRegister(MCP_EFLG, EFLG_RX0OVR | EFLG_RX1OVR, 0);
    //clearInterrupts();
    modifyRegister(MCP_CANINTF, CANINTF_ERRIF, 0);
}

uint8_t MCP2515::errorCountRX(void)                             
{
    return readRegister(MCP_REC);
}

uint8_t MCP2515::errorCountTX(void)                             
{
    return readRegister(MCP_TEC);
}
