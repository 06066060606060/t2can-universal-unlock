#pragma once

// TMR: A = Body/J2, B = Chassis (VH on YL)/J3, C = Party/J4.
// YL has no Body bus; retain upstream's Party + VH profile on C + B.
static SemaphoreHandle_t boardCanMutex;
struct BoardCanLock {
  bool locked;
  BoardCanLock(TickType_t wait = portMAX_DELAY) : locked(xSemaphoreTake(boardCanMutex, wait) == pdTRUE) {}
  ~BoardCanLock() { if (locked) xSemaphoreGive(boardCanMutex); }
};

static bool boardTripleCan() {
  return board == BOARD_TMR && activeVehicleTopology == VEHICLE_TOPOLOGY_STANDARD_THREE_CAN;
}
static volatile uint32_t boardPartyRx, boardPartyLastRx;

static MCP2515 &boardCanParty() {
  static MCP2515 party(8, 10000000, &SPI);
  return party;
}

static MCP2515 &boardCanA() {
  // MCP2515 constructors drive CS: instantiate only after board detection.
  if (board == BOARD_TMR && activeCanAIsParty()) {
    return boardCanParty();
  }
  static MCP2515 body(10, 10000000, &SPI);
  return body;
}

struct BoardCanA {
  template <typename F> MCP2515::ERROR configure(F operation) {
    const auto result = operation(boardCanA());
    return result == MCP2515::ERROR_OK && boardTripleCan() ? operation(boardCanParty()) : result;
  }
  MCP2515::ERROR reset() {
    BoardCanLock lock;
    if (board == BOARD_TMR) { digitalWrite(18, HIGH); digitalWrite(21, HIGH); }
    return configure([](MCP2515 &can) { return can.reset(); });
  }
  MCP2515::ERROR setBitrate(CAN_SPEED speed, CAN_CLOCK clock) {
    BoardCanLock lock;
    return configure([=](MCP2515 &can) { return can.setBitrate(speed, clock); });
  }
  MCP2515::ERROR setNormalMode() {
    BoardCanLock lock;
    const auto result = configure([](MCP2515 &can) { return can.setNormalMode(); });
    if (board == BOARD_TMR && result == MCP2515::ERROR_OK) {
      digitalWrite(activeCanAIsParty() ? 21 : 18, LOW);
      if (boardTripleCan()) digitalWrite(21, LOW);
    }
    return result;
  }
  MCP2515::ERROR readMessage(can_frame *frame, bool *party) {
    BoardCanLock lock;
    if (boardTripleCan()) {
      static bool nextParty;
      for (int i = 0; i < 2; ++i) {
        *party = nextParty;
        nextParty = !nextParty;
        auto &can = *party ? boardCanParty() : boardCanA();
        const auto result = can.readMessage(frame);
        if (result == MCP2515::ERROR_OK) return result;
      }
      return MCP2515::ERROR_NOMSG;
    }
    *party = activeCanAIsParty();
    return boardCanA().readMessage(frame);
  }
  MCP2515::ERROR sendMessage(const can_frame *frame, bool party) {
    BoardCanLock lock(0);
    if (!lock.locked) return MCP2515::ERROR_ALLTXBUSY;
    return (party && board == BOARD_TMR ? boardCanParty() : boardCanA()).sendMessage(frame);
  }
  uint8_t getErrorFlags() {
    BoardCanLock lock;
    return boardCanA().getErrorFlags() | (boardTripleCan() ? boardCanParty().getErrorFlags() : 0);
  }
  void clearRXnOVR() {
    BoardCanLock lock;
    if (board == BOARD_T2CAN) { boardCanA().clearRXnOVR(); return; }
    boardCanA().clearRXnOVRFlags(); // Preserve unrelated RX/TX interrupt flags.
    if (boardTripleCan()) boardCanParty().clearRXnOVRFlags();
  }
};

static MCP2515 *boardCanB;
static QueueHandle_t boardRx, boardTx;
static TaskHandle_t boardCanTask;
static twai_status_info_t boardStatus = {};
static bool boardInstalled, boardTxPending;
static uint32_t boardAlerts, boardAlertMask, boardTxSince;

static void ARDUINO_ISR_ATTR boardCanWake() {
  BaseType_t woken = pdFALSE;
  if (boardCanTask) vTaskNotifyGiveFromISR(boardCanTask, &woken);
  if (woken) portYIELD_FROM_ISR();
}

static bool boardCanBegin() {
  boardCanMutex = xSemaphoreCreateMutex();
  return boardCanMutex != nullptr;
}

// All controller calls, including multi-transaction MCP library operations,
// share this mutex. Never wait for a queue while holding it.
static void boardCanPollOnce() {
  BoardCanLock lock;
  if (boardInstalled && boardStatus.state == TWAI_STATE_RUNNING) {
    const uint8_t errors = boardCanB->getErrorFlags();
    boardStatus.tx_error_counter = boardCanB->errorCountTX();
    boardStatus.rx_error_counter = boardCanB->errorCountRX();
    if (errors & (MCP2515::EFLG_RX0OVR | MCP2515::EFLG_RX1OVR)) {
      boardStatus.rx_overrun_count++;
      boardAlerts |= TWAI_ALERT_RX_QUEUE_FULL;
      boardCanB->clearRXnOVRFlags();
    }
    // Missing ACK can leave TXREQ set indefinitely without reaching bus-off.
    if ((errors & MCP2515::EFLG_TXBO) ||
        (boardTxPending && (uint32_t)(millis() - boardTxSince) >= 300)) {
      digitalWrite(14, HIGH);
      boardStatus.state = TWAI_STATE_BUS_OFF;
      boardAlerts |= TWAI_ALERT_BUS_OFF;
      if (boardTxPending) boardStatus.tx_failed_count++;
      boardTxPending = false;
      xQueueReset(boardTx);
      xQueueReset(boardRx);
    } else {
      for (uint8_t i = 0; i < 32; ++i) {
        can_frame frame;
        if (boardCanB->readMessage(&frame) != MCP2515::ERROR_OK) break;
        twai_message_t msg = {};
        msg.extd = (frame.can_id & CAN_EFF_FLAG) != 0;
        msg.rtr = (frame.can_id & CAN_RTR_FLAG) != 0;
        msg.identifier = frame.can_id & (msg.extd ? CAN_EFF_MASK : CAN_SFF_MASK);
        msg.data_length_code = frame.can_dlc;
        memcpy(msg.data, frame.data, frame.can_dlc);
        if (xQueueSend(boardRx, &msg, 0) == pdTRUE) boardAlerts |= TWAI_ALERT_RX_DATA;
        else {
          boardStatus.rx_missed_count++;
          boardAlerts |= TWAI_ALERT_RX_QUEUE_FULL;
        }
      }
      boardCanB->clearERRIF();
      boardCanB->clearMERR();
      // One hardware mailbox preserves the upstream FIFO order. Enqueue
      // success is not TX success: only TX0IF confirms completion.
      const uint8_t status = boardCanB->getStatus();
      if (boardTxPending && !(status & 0x04)) { // TX0REQ
        const bool ok = (status & 0x08) != 0; // TX0IF
        boardAlerts |= ok ? TWAI_ALERT_TX_SUCCESS : TWAI_ALERT_TX_FAILED;
        if (!ok) boardStatus.tx_failed_count++;
        boardTxPending = false;
        boardCanB->clearTXInterrupts();
        if (!uxQueueMessagesWaiting(boardTx)) boardAlerts |= TWAI_ALERT_TX_IDLE;
      }
      can_frame frame;
      if (!boardTxPending && xQueueReceive(boardTx, &frame, 0) == pdTRUE) {
        boardCanB->clearTXInterrupts();
        const auto result = boardCanB->sendMessage(MCP2515::TXB0, &frame);
        // The library may report an immediate bus error after setting
        // TXREQ; retain ownership until completion/recovery in that case.
        boardTxPending = result == MCP2515::ERROR_OK || (boardCanB->getStatus() & 0x04);
        boardTxSince = millis();
        if (!boardTxPending) {
          boardStatus.tx_failed_count++;
          boardAlerts |= TWAI_ALERT_TX_FAILED;
        }
      }
    }
  }
}

static void boardCanPoll(void *) {
  for (;;) {
    boardCanPollOnce();
    ulTaskNotifyTake(pdTRUE, 1); // IRQ/queued TX wakes immediately; timeout services TX completion.
  }
}

static esp_err_t boardTwaiInstall(const twai_general_config_t *g,
                                 const twai_timing_config_t *t, const twai_filter_config_t *f) {
  if (board != BOARD_TMR) return twai_driver_install(g, t, f);
  BoardCanLock lock;
  if (boardInstalled) return ESP_ERR_INVALID_STATE;
  if (!boardRx) boardRx = xQueueCreate(g->rx_queue_len, sizeof(twai_message_t));
  if (!boardTx) boardTx = xQueueCreate(g->tx_queue_len, sizeof(can_frame));
  if (!boardRx || !boardTx) return ESP_ERR_NO_MEM;
  static MCP2515 chassis(9, 10000000, &SPI);
  boardCanB = &chassis;
  if (!boardCanTask && xTaskCreatePinnedToCore(boardCanPoll, "canSPI", 3072, nullptr,
                                             6, &boardCanTask, 1) != pdPASS)
    return ESP_ERR_NO_MEM;
  pinMode(5, INPUT_PULLUP); // A23 IRQ_B, MCP2515 active-low interrupt.
  attachInterrupt(digitalPinToInterrupt(5), boardCanWake, FALLING);
  boardStatus = {};
  boardStatus.state = TWAI_STATE_STOPPED;
  boardAlerts = 0;
  boardInstalled = true;
  return ESP_OK;
}

static esp_err_t boardTwaiStart() {
  if (board != BOARD_TMR) return twai_start();
  BoardCanLock lock;
  if (!boardInstalled || boardStatus.state != TWAI_STATE_STOPPED) return ESP_ERR_INVALID_STATE;
  digitalWrite(14, HIGH);
  xQueueReset(boardRx);
  xQueueReset(boardTx);
  boardTxPending = false;
  if (boardCanB->reset() != MCP2515::ERROR_OK ||
      boardCanB->setBitrate(CAN_500KBPS, MCP_16MHZ) != MCP2515::ERROR_OK ||
      boardCanB->setNormalMode() != MCP2515::ERROR_OK) return ESP_FAIL;
  boardAlerts = TWAI_ALERT_TX_IDLE;
  boardStatus.state = TWAI_STATE_RUNNING;
  digitalWrite(14, LOW);
  return ESP_OK;
}

static esp_err_t boardTwaiStop() {
  if (board != BOARD_TMR) return twai_stop();
  BoardCanLock lock;
  if (!boardInstalled) return ESP_ERR_INVALID_STATE;
  digitalWrite(14, HIGH);
  boardTxPending = false;
  xQueueReset(boardTx);
  xQueueReset(boardRx);
  // SPI reset also aborts unacknowledged TX before transceiver re-enable.
  const auto result = boardCanB->reset();
  boardStatus.state = TWAI_STATE_STOPPED;
  boardAlerts = 0;
  return result == MCP2515::ERROR_OK ? ESP_OK : ESP_FAIL;
}

static esp_err_t boardTwaiUninstall() {
  if (board != BOARD_TMR) return twai_driver_uninstall();
  const esp_err_t result = boardTwaiStop();
  BoardCanLock lock;
  boardInstalled = false;
  return result;
}

static esp_err_t boardTwaiRecover() {
  if (board != BOARD_TMR) return twai_initiate_recovery();
  // Upstream invalidates feature epochs before this call and restarts STOPPED.
  return boardTwaiStop();
}

static esp_err_t boardTwaiTransmit(const twai_message_t *msg, TickType_t wait) {
  if (board != BOARD_TMR) return twai_transmit(msg, wait);
  if (!msg || msg->data_length_code > 8 || msg->self || msg->ss ||
      msg->identifier > (msg->extd ? CAN_EFF_MASK : CAN_SFF_MASK)) return ESP_ERR_INVALID_ARG;
  can_frame frame = {};
  frame.can_id = msg->identifier | (msg->extd ? CAN_EFF_FLAG : 0) | (msg->rtr ? CAN_RTR_FLAG : 0);
  frame.can_dlc = msg->data_length_code;
  memcpy(frame.data, msg->data, frame.can_dlc);
  const TickType_t start = xTaskGetTickCount();
  do {
    {
      BoardCanLock lock(0);
      if (!lock.locked) return ESP_ERR_TIMEOUT;
      if (!boardInstalled || boardStatus.state != TWAI_STATE_RUNNING) return ESP_ERR_INVALID_STATE;
      if (xQueueSend(boardTx, &frame, 0) == pdTRUE) {
        xTaskNotifyGive(boardCanTask);
        return ESP_OK;
      }
    }
    if ((TickType_t)(xTaskGetTickCount() - start) >= wait) break;
    vTaskDelay(1);
  } while (true);
  return ESP_ERR_TIMEOUT;
}

static esp_err_t boardTwaiReceive(twai_message_t *msg, TickType_t wait) {
  if (board != BOARD_TMR) return twai_receive(msg, wait);
  if (!msg) return ESP_ERR_INVALID_ARG;
  {
    BoardCanLock lock;
    if (!boardInstalled || boardStatus.state != TWAI_STATE_RUNNING) return ESP_ERR_INVALID_STATE;
  }
  return xQueueReceive(boardRx, msg, wait) == pdTRUE ? ESP_OK : ESP_ERR_TIMEOUT;
}

static esp_err_t boardTwaiStatus(twai_status_info_t *status) {
  if (board != BOARD_TMR) return twai_get_status_info(status);
  if (!status || !boardCanMutex) return ESP_ERR_INVALID_STATE;
  BoardCanLock lock;
  if (!boardInstalled) return ESP_ERR_INVALID_STATE;
  *status = boardStatus;
  status->msgs_to_rx = uxQueueMessagesWaiting(boardRx);
  status->msgs_to_tx = uxQueueMessagesWaiting(boardTx) + boardTxPending;
  return ESP_OK;
}

static esp_err_t boardTwaiClearTx() {
  if (board != BOARD_TMR) return twai_clear_transmit_queue();
  BoardCanLock lock;
  if (!boardInstalled) return ESP_ERR_INVALID_STATE;
  xQueueReset(boardTx); // Match TWAI: do not abort the current hardware frame.
  return ESP_OK;
}

static esp_err_t boardTwaiConfigureAlerts(uint32_t mask, uint32_t *current) {
  if (board != BOARD_TMR) return twai_reconfigure_alerts(mask, current);
  BoardCanLock lock;
  if (current) *current = boardAlerts & boardAlertMask;
  boardAlertMask = mask;
  boardAlerts = 0;
  return ESP_OK;
}

static esp_err_t boardTwaiReadAlerts(uint32_t *alerts, TickType_t wait) {
  if (board != BOARD_TMR) return twai_read_alerts(alerts, wait);
  BoardCanLock lock;
  *alerts = boardAlerts & boardAlertMask;
  boardAlerts = 0;
  return *alerts ? ESP_OK : ESP_ERR_TIMEOUT; // Upstream only polls with wait=0.
}

// Adapt calls, not driver types or vehicle logic. T-2CAN still calls native TWAI.
#define twai_driver_install boardTwaiInstall
#define twai_driver_uninstall boardTwaiUninstall
#define twai_start boardTwaiStart
#define twai_stop boardTwaiStop
#define twai_initiate_recovery boardTwaiRecover
#define twai_transmit boardTwaiTransmit
#define twai_receive boardTwaiReceive
#define twai_get_status_info boardTwaiStatus
#define twai_clear_transmit_queue boardTwaiClearTx
#define twai_reconfigure_alerts boardTwaiConfigureAlerts
#define twai_read_alerts boardTwaiReadAlerts
