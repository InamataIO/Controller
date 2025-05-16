#ifdef GSM_NETWORK

#include "managers/logging.h"
#include "network_client_impl.h"

NetworkClient::NetworkClient() : _impl(new NetworkClient::Impl()) {
  TRACELN("NC()");
}
NetworkClient::NetworkClient(WiFiClient wifi_client)
    : _impl(new NetworkClient::Impl()) {
  TRACELN("NC(WC)");
}
NetworkClient::~NetworkClient() { TRACELN("~NC()"); }

int NetworkClient::connect(IPAddress ip, uint16_t port) {
  if (_impl->gsm_client_) {
    return _impl->gsm_client_->connect(ip, port);
  } else if (_impl->wifi_client_) {
    return _impl->wifi_client_->connect(ip, port);
  }
  TRACELN(_impl->no_interface_error_);
  return 0;
}

int NetworkClient::connect(const char *host, uint16_t port) {
  if (_impl->gsm_client_) {
    return _impl->gsm_client_->connect(host, port);
  } else if (_impl->wifi_client_) {
    return _impl->wifi_client_->connect(host, port);
  }
  TRACELN(_impl->no_interface_error_);
  return 0;
}
int NetworkClient::connect(const char *host, uint16_t port,
                           int32_t timeout_ms) {
  if (_impl->gsm_client_) {
    return _impl->gsm_client_->connect(host, port, timeout_ms);
  } else if (_impl->wifi_client_) {
    return _impl->wifi_client_->connect(host, port, timeout_ms);
  }
  TRACELN(_impl->no_interface_error_);
  return 0;
}

size_t NetworkClient::write(uint8_t data) {
  if (_impl->gsm_client_) {
    return _impl->gsm_client_->write(data);
  } else if (_impl->wifi_client_) {
    return _impl->wifi_client_->write(data);
  }
  TRACELN(_impl->no_interface_error_);
  return 0;
}

size_t NetworkClient::write(const uint8_t *buf, size_t size) {
  Serial.printf("Send_: %zu\n", size);
  if (_impl->gsm_client_) {
    return _impl->gsm_client_->write(buf, size);
  } else if (_impl->wifi_client_) {
    return _impl->wifi_client_->write(buf, size);
  }
  TRACELN(_impl->no_interface_error_);
  return 0;
}

size_t NetworkClient::write(const char *str) {
  const int size = strlen(str);
  Serial.printf("Send: %zu\n", size);
  if (_impl->gsm_client_) {
    return _impl->gsm_client_->write((const uint8_t *)str, size);
  } else if (_impl->wifi_client_) {
    return _impl->wifi_client_->write((const uint8_t *)str, size);
  }
  TRACELN(_impl->no_interface_error_);
  return 0;
}

int NetworkClient::available() {
  if (_impl->gsm_client_) {
    return _impl->gsm_client_->available();
  } else if (_impl->wifi_client_) {
    return _impl->wifi_client_->available();
  }
  TRACELN(_impl->no_interface_error_);
  return 0;
}

int NetworkClient::read() {
  if (_impl->gsm_client_) {
    return _impl->gsm_client_->read();
  } else if (_impl->wifi_client_) {
    return _impl->wifi_client_->read();
  }
  TRACELN(_impl->no_interface_error_);
  return 0;
}

int NetworkClient::read(uint8_t *buf, size_t size) {
  if (_impl->gsm_client_) {
    return _impl->gsm_client_->read(buf, size);
  } else if (_impl->wifi_client_) {
    return _impl->wifi_client_->read(buf, size);
  }
  TRACELN(_impl->no_interface_error_);
  return 0;
}

int NetworkClient::peek() {
  if (_impl->gsm_client_) {
    return _impl->gsm_client_->peek();
  } else if (_impl->wifi_client_) {
    return _impl->wifi_client_->peek();
  }
  TRACELN(_impl->no_interface_error_);
  return 0;
}

void NetworkClient::flush() {
  if (_impl->gsm_client_) {
    return _impl->gsm_client_->flush();
  } else if (_impl->wifi_client_) {
    return _impl->wifi_client_->flush();
  }
  TRACELN(_impl->no_interface_error_);
}

void NetworkClient::stop() {
  if (_impl->gsm_client_) {
    return _impl->gsm_client_->stop();
  } else if (_impl->wifi_client_) {
    return _impl->wifi_client_->stop();
  }
  TRACELN(_impl->no_interface_error_);
}

uint8_t NetworkClient::connected() {
  if (_impl->gsm_client_) {
    return _impl->gsm_client_->connected();
  } else if (_impl->wifi_client_) {
    return _impl->wifi_client_->connected();
  }
  TRACELN(_impl->no_interface_error_);
  return 0;
}

NetworkClient::operator bool() {
  if (_impl->gsm_client_) {
    return _impl->gsm_client_->operator bool();
  } else if (_impl->wifi_client_) {
    return _impl->wifi_client_->operator bool();
  }
  TRACELN(_impl->no_interface_error_);
  return 0;
}

#endif