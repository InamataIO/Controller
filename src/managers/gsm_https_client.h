#pragma once

#include <Arduino.h>
#include <ArduinoHttpClient.h>
#include <SSLClient.h>
#include <TinyGSM.h>

#include <memory>

struct GsmHttpsClient {
  GsmHttpsClient(TinyGsm& tiny_gsm, const char* server_name, uint16_t port,
                 uint8_t mux, bool secure);
  ~GsmHttpsClient() = default;

  String server_name_;

  std::unique_ptr<TinyGsmClient> gsm_client_;
  // std::unique_ptr<SSLClient> ssl_client_;
  std::unique_ptr<HttpClient> http_client_;
};
