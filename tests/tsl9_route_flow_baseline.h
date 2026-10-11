// Frozen v3.28.4 route procedures for differential host testing.
static void tsl9InputObserveCanA(const struct can_frame &incoming) {
  if (!activeProfileTsl9InputOnBodyCanA() || incoming.can_dlc < 8 ||
      tsl9InputMuxPure(incoming.data) != TSL9_INPUT_MUX1_PURE) return;
  const uint32_t now = (uint32_t)millis();
  const int8_t left = tsl9InputDecodeSignedSixBitPure(incoming.data[2]);
  const int8_t right = tsl9InputDecodeSignedSixBitPure(incoming.data[3]);
  portENTER_CRITICAL(&tsl9InputMux);
  tsl9InputCanATemplate = incoming;
  tsl9InputCanATemplateValid = true;
  tsl9InputCanATemplateMs = now;
  tsl9InputMux1Rx++;
  tsl9InputScheduler.observeManualTicks(left, right, now);
  portEXIT_CRITICAL(&tsl9InputMux);
}

static void tsl9InputObserveCanB(const twai_message_t &incoming) {
  if (activeProfileTsl9InputOnBodyCanA() ||
      !activeProfileTsl9InputSupported() || incoming.data_length_code < 8 ||
      tsl9InputMuxPure(incoming.data) != TSL9_INPUT_MUX1_PURE) return;
  const uint32_t now = (uint32_t)millis();
  const int8_t left = tsl9InputDecodeSignedSixBitPure(incoming.data[2]);
  const int8_t right = tsl9InputDecodeSignedSixBitPure(incoming.data[3]);
  portENTER_CRITICAL(&tsl9InputMux);
  tsl9InputCanBTemplate = incoming;
  tsl9InputCanBTemplateValid = true;
  tsl9InputCanBTemplateMs = now;
  tsl9InputMux1Rx++;
  tsl9InputScheduler.observeManualTicks(left, right, now);
  portEXIT_CRITICAL(&tsl9InputMux);
}

static void tsl9InputServiceCanA() {
  if (continuousApOwnsInputRoute(ContApRoute::BodyA)) return;
  if (!activeProfileTsl9InputOnBodyCanA()) return;
  const uint32_t now = (uint32_t)millis();
  Tsl9InputInputsPure in = tsl9InputInputsSnapshot(now, true);
  Tsl9InputCommandPure command = {};
  struct can_frame stock = {};
  bool haveTemplate = false;
  portENTER_CRITICAL(&tsl9InputMux);
  haveTemplate = tsl9InputCanATemplateValid;
  in.templateFresh = haveTemplate &&
      (uint32_t)(now - tsl9InputCanATemplateMs) <=
          tsl9InputTemplateMaxAgeMsPure(in.mode);
  command = tsl9InputScheduler.service(in);
  if (haveTemplate) stock = tsl9InputCanATemplate;
  portEXIT_CRITICAL(&tsl9InputMux);
  if (command.kind == TSL9_INPUT_COMMAND_NONE_PURE) return;
  const bool ok = haveTemplate && in.templateFresh &&
      tsl9InputSendCanA(stock, command);
  Tsl9InputCommandPure cleanup = {};
  const uint32_t randomValue = command.kind == TSL9_INPUT_COMMAND_STEP_PURE
      ? esp_random() : 0u;
  portENTER_CRITICAL(&tsl9InputMux);
  if (ok) tsl9InputTxOk++; else tsl9InputTxFail++;
  if (command.kind == TSL9_INPUT_COMMAND_CENTER_PURE) tsl9InputCleanupTx++;
  cleanup = tsl9InputScheduler.onCommandResult(
      command, ok, (uint32_t)millis(), randomValue);
  portEXIT_CRITICAL(&tsl9InputMux);
  if (cleanup.kind != TSL9_INPUT_COMMAND_NONE_PURE) {
    const bool cleanupOk = haveTemplate && in.templateFresh &&
        tsl9InputSendCanA(stock, cleanup);
    portENTER_CRITICAL(&tsl9InputMux);
    if (cleanupOk) tsl9InputTxOk++; else tsl9InputTxFail++;
    tsl9InputCleanupTx++;
    (void)tsl9InputScheduler.onCommandResult(
        cleanup, cleanupOk, (uint32_t)millis(), 0u);
    portEXIT_CRITICAL(&tsl9InputMux);
  }
}

static void tsl9InputServiceCanB() {
  if (continuousApOwnsInputRoute(ContApRoute::ChassisB)) return;
  if (activeProfileTsl9InputOnBodyCanA() ||
      !activeProfileTsl9InputSupported()) return;
  const uint32_t now = (uint32_t)millis();
  Tsl9InputInputsPure in = tsl9InputInputsSnapshot(now, false);
  Tsl9InputCommandPure command = {};
  twai_message_t stock = {};
  bool haveTemplate = false;
  portENTER_CRITICAL(&tsl9InputMux);
  haveTemplate = tsl9InputCanBTemplateValid;
  in.templateFresh = haveTemplate &&
      (uint32_t)(now - tsl9InputCanBTemplateMs) <=
          tsl9InputTemplateMaxAgeMsPure(in.mode);
  command = tsl9InputScheduler.service(in);
  if (haveTemplate) stock = tsl9InputCanBTemplate;
  portEXIT_CRITICAL(&tsl9InputMux);
  if (command.kind == TSL9_INPUT_COMMAND_NONE_PURE) return;
  const bool ok = haveTemplate && in.templateFresh &&
      tsl9InputSendCanB(stock, command);
  Tsl9InputCommandPure cleanup = {};
  const uint32_t randomValue = command.kind == TSL9_INPUT_COMMAND_STEP_PURE
      ? esp_random() : 0u;
  portENTER_CRITICAL(&tsl9InputMux);
  if (ok) tsl9InputTxOk++; else tsl9InputTxFail++;
  if (command.kind == TSL9_INPUT_COMMAND_CENTER_PURE) tsl9InputCleanupTx++;
  cleanup = tsl9InputScheduler.onCommandResult(
      command, ok, (uint32_t)millis(), randomValue);
  portEXIT_CRITICAL(&tsl9InputMux);
  if (cleanup.kind != TSL9_INPUT_COMMAND_NONE_PURE) {
    const bool cleanupOk = haveTemplate && in.templateFresh &&
        tsl9InputSendCanB(stock, cleanup);
    portENTER_CRITICAL(&tsl9InputMux);
    if (cleanupOk) tsl9InputTxOk++; else tsl9InputTxFail++;
    tsl9InputCleanupTx++;
    (void)tsl9InputScheduler.onCommandResult(
        cleanup, cleanupOk, (uint32_t)millis(), 0u);
    portEXIT_CRITICAL(&tsl9InputMux);
  }
}
