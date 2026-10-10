#pragma once

static size_t canUsbFormatControllers(char *line, size_t capacity, uint32_t session) {
  twai_status_info_t status = {};
  const uint32_t epoch = __atomic_load_n(&canUsbControllerEpoch, __ATOMIC_ACQUIRE);
  const bool valid = !canSubsystemBusy && twai_get_status_info(&status) == ESP_OK &&
      epoch == __atomic_load_n(&canUsbControllerEpoch, __ATOMIC_ACQUIRE);
  return (size_t)snprintf(line, capacity, "@CTRL,%lu,%lu,%lu,%lu,%lu,%u\n",
      (unsigned long)session, (unsigned long)epoch,
      (unsigned long)__atomic_load_n(&canUsbMcpOverflowTotal, __ATOMIC_RELAXED),
      (unsigned long)(valid ? status.rx_missed_count : 0),
      (unsigned long)(valid ? status.rx_overrun_count : 0), valid ? 1U : 0U);
}

static void canUsbTask(void *) {
  char command[64] = {}, line[160] = {}, initialControllers[160] = {};
  size_t commandLength = 0, length = 0, offset = 0;
  bool commandOverflow = false, stopping = false;
  uint64_t emitted = 0;
  uint32_t lastStatus = 0;
  uint8_t burst = 0;
  bool wasConnected = false, controllersDue = false, reportDue = false;
  // This task is the sole protocol writer; diagnostics builds must not mix it.
  for (;;) {
    if (!Serial) {
      // The hardware mode is deliberately retained on unplug, including PASSIVE.
      portENTER_CRITICAL(&canUsbMux);
      canUsbQueue.active = false;
      canUsbQueue.count = 0;
      __atomic_store_n(&canUsbCapturing, false, __ATOMIC_RELEASE);
      portEXIT_CRITICAL(&canUsbMux);
      length = offset = commandLength = 0;
      commandOverflow = stopping = controllersDue = reportDue = false;
      if (wasConnected) Serial.flush(); // HW CDC discards TX buffering when disconnected
      wasConnected = false;
      vTaskDelay(pdMS_TO_TICKS(10));
      continue;
    }
    if (!wasConnected) {
      wasConnected = true;
      length = (size_t)snprintf(line, sizeof(line), "\n@HELLO,1,%s\n", FW_VERSION);
    }
    // Send a complete line before accepting commands or selecting the next line.
    if (length) {
      const int space = Serial.availableForWrite();
      if (space > 0) {
        size_t chunk = length - offset;
        if (chunk > (size_t)space) chunk = (size_t)space;
        offset += Serial.write((const uint8_t *)line + offset, chunk);
        if (offset == length) { length = offset = 0; }
      }
      if (length || ++burst >= 32) {
        burst = 0;
        vTaskDelay(pdMS_TO_TICKS(1));
      }
      continue;
    }
    size_t budget = 64;
    while (budget-- && Serial.available()) {
      const char c = (char)Serial.read();
      if (c == '\r') continue;
      if (c != '\n') {
        if (commandLength + 1 < sizeof(command)) command[commandLength++] = c;
        else commandOverflow = true;
        continue;
      }
      command[commandLength] = 0;
      if (commandOverflow) {
        length = (size_t)snprintf(line, sizeof(line), "@ERROR,command_too_long\n");
      } else if (strcmp(command, "LOGGER HELLO") == 0) {
        length = (size_t)snprintf(line, sizeof(line), "@HELLO,1,%s\n", FW_VERSION);
      } else if (strcmp(command, "LOGGER START PASSIVE") == 0 ||
                 strcmp(command, "LOGGER START ACTIVE") == 0) {
        bool active; size_t pending;
        portENTER_CRITICAL(&canUsbMux);
        active = canUsbQueue.active; pending = canUsbQueue.count;
        portEXIT_CRITICAL(&canUsbMux);
        if (active || pending || stopping) {
          length = (size_t)snprintf(line, sizeof(line), "@ERROR,session_busy\n");
        } else {
          const bool passive = strcmp(command, "LOGGER START PASSIVE") == 0;
          if (!canUsbRequestMode(passive)) {
            length = (size_t)snprintf(line, sizeof(line), "@ERROR,mode_transition_failed\n");
          } else {
            // Baseline precedes capture admission, so early controller loss is
            // not hidden in a snapshot taken after frames have already queued.
            uint32_t nextSession = canUsbQueue.session + 1U;
            if (!nextSession) ++nextSession;
            canUsbFormatControllers(initialControllers, sizeof(initialControllers), nextSession);
            portENTER_CRITICAL(&canUsbMux);
            canUsbQueue.start(); emitted = 0;
            __atomic_store_n(&canUsbCapturing, true, __ATOMIC_RELEASE);
            const uint32_t session = canUsbQueue.session;
            portEXIT_CRITICAL(&canUsbMux);
            length = (size_t)snprintf(line, sizeof(line), "@START,%lu,%s\n",
                (unsigned long)session, passive ? "PASSIVE" : "ACTIVE");
            lastStatus = (uint32_t)millis();
            controllersDue = true;
          }
        }
      } else if (strcmp(command, "LOGGER STOP") == 0) {
        portENTER_CRITICAL(&canUsbMux);
        canUsbQueue.active = false;
        __atomic_store_n(&canUsbCapturing, false, __ATOMIC_RELEASE);
        portEXIT_CRITICAL(&canUsbMux);
        stopping = true;
      } else if (strcmp(command, "LOGGER STATUS") == 0) {
        lastStatus = (uint32_t)millis() - 1000U;
      } else {
        length = (size_t)snprintf(line, sizeof(line), "@ERROR,unknown_command\n");
      }
      commandLength = 0; commandOverflow = false;
      if (length) break;
    }
    if (length) continue;
    const uint32_t now = (uint32_t)millis();
    uint32_t session; uint64_t seen, queued, dropped; size_t pending;
    bool active;
    portENTER_CRITICAL(&canUsbMux);
    session = canUsbQueue.session; seen = canUsbQueue.seen;
    queued = canUsbQueue.queued; dropped = canUsbQueue.dropped;
    pending = canUsbQueue.count; active = canUsbQueue.active;
    portEXIT_CRITICAL(&canUsbMux);
    if (controllersDue || (!reportDue && ((active && (uint32_t)(now - lastStatus) >= 1000U) || (stopping && !pending)))) {
      if (controllersDue) {
        length = strlen(initialControllers);
        memcpy(line, initialControllers, length + 1U);
      } else length = canUsbFormatControllers(line, sizeof(line), session);
      reportDue = !controllersDue;
      controllersDue = false;
      continue;
    }
    if (reportDue) {
      const bool finalStop = stopping && !pending;
      length = (size_t)snprintf(line, sizeof(line), "@%s,%lu,%llu,%llu,%llu,%llu\n",
          finalStop ? "STOP" : "STAT", (unsigned long)session,
          (unsigned long long)seen, (unsigned long long)queued,
          (unsigned long long)dropped, (unsigned long long)(queued - emitted));
      if (finalStop) stopping = false;
      reportDue = false; lastStatus = now;
      continue;
    }
    CanUsbFrame frame = {};
    portENTER_CRITICAL(&canUsbMux);
    const bool popped = canUsbQueue.pop(frame);
    portEXIT_CRITICAL(&canUsbMux);
    if (popped) {
      length = canUsbFormatFrame(line, sizeof(line), frame);
      if (length) ++emitted; // next selection happens only after the line is sent
    } else vTaskDelay(pdMS_TO_TICKS(2));
  }
}
