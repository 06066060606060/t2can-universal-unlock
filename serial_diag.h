#pragma once

#if T2CAN_SERIAL_DIAGNOSTICS
  #define T2CAN_SERIAL_BEGIN(...)   Serial.begin(__VA_ARGS__)
  #define T2CAN_SERIAL_PRINTF(...)  Serial.printf(__VA_ARGS__)
  #define T2CAN_SERIAL_PRINTLN(...) Serial.println(__VA_ARGS__)
  #define T2CAN_SERIAL_PRINT(...)   Serial.print(__VA_ARGS__)
#else
  #define T2CAN_SERIAL_BEGIN(...)   do {} while (0)
  #define T2CAN_SERIAL_PRINTF(...)  do {} while (0)
  #define T2CAN_SERIAL_PRINTLN(...) do {} while (0)
  #define T2CAN_SERIAL_PRINT(...)   do {} while (0)
#endif
