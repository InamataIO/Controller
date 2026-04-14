#ifdef GSM_NETWORK

#include "gsm_https_client.h"

GsmHttpsClient::GsmHttpsClient(TinyGsm& tiny_gsm, const char* server_name,
                               uint16_t port, uint8_t mux, bool secure)
    : server_name_(server_name) {
  gsm_client_ = std::make_unique<TinyGsmClient>(tiny_gsm, mux);
  // if (secure) {
  //   ssl_client_ = std::make_unique<SSLClient>(gsm_client_.get());
  //   http_client_ = std::make_unique<HttpClient>(*ssl_client_.get(),
  //                                               server_name_.c_str(), port);
  // } else {
  http_client_ = std::make_unique<HttpClient>(*gsm_client_.get(),
                                              server_name_.c_str(), port);
  // }
}

#endif