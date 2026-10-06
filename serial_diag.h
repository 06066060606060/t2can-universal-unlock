#pragma once

#if TMR_SERIAL_DIAGNOSTICS
  #define TMR_SERIAL_BEGIN(...)   Serial.begin(__VA_ARGS__)
  #define TMR_SERIAL_PRINTF(...)  Serial.printf(__VA_ARGS__)
  #define TMR_SERIAL_PRINTLN(...) Serial.println(__VA_ARGS__)
  #define TMR_SERIAL_PRINT(...)   Serial.print(__VA_ARGS__)
#else
  #define TMR_SERIAL_BEGIN(...)   do {} while (0)
  #define TMR_SERIAL_PRINTF(...)  do {} while (0)
  #define TMR_SERIAL_PRINTLN(...) do {} while (0)
  #define TMR_SERIAL_PRINT(...)   do {} while (0)
#endif
