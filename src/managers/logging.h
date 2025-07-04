#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

#ifdef ENABLE_TRACE
#define TRACEF(format, ...)                     \
  Serial.printf("%s(%d) ", __FILE__, __LINE__); \
  Serial.printf(String(F(format)).c_str(), __VA_ARGS__)
#define TRACELN(msg)                                              \
  Serial.printf("%s::%s(%d) ", __FILE__, __FUNCTION__, __LINE__); \
  Serial.println(msg)
#define TRACESTACK                                         \
  Serial.printf("%s(%d) stack %u\r\n", __FILE__, __LINE__, \
                ESP.getFreeContStack());
#define TRACEJSON(json)        \
  serializeJson(json, Serial); \
  Serial.println()
#define TRACEKJSON(key, json)  \
  Serial.print(key);           \
  serializeJson(json, Serial); \
  Serial.println()
#else
#define TRACEF(format, ...) (void)sizeof(__VA_ARGS__)
#define TRACELN(format)
#define TRACESTACK
#define TRACEJSON(json)
#define TRACEKJSON(key, json)
#endif
