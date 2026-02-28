#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <utility>

namespace cpp_serial {

enum class Parity {
  None,
  Odd,
  Even
};

enum class StopBits {
  One,
  Two
};

enum class FlowControl {
  None,
  RtsCts
};

enum class ErrorCode {
  Ok = 0,
  InvalidArgument,
  InvalidState,
  OpenFailed,
  ReadFailed,
  WriteFailed,
  CloseFailed,
  Unsupported,
  InternalError
};

struct SerialConfig {
  std::uint32_t baudRate{115200};
  std::uint8_t dataBits{8};
  Parity parity{Parity::None};
  StopBits stopBits{StopBits::One};
  FlowControl flowControl{FlowControl::None};
  std::chrono::milliseconds readTimeout{100};
  std::chrono::milliseconds writeTimeout{100};
};

struct Status {
  bool ok{true};
  ErrorCode code{ErrorCode::Ok};
  std::string message{};

  static Status success() { return {}; }

  static Status failure(ErrorCode code, std::string msg) {
    return {.ok = false, .code = code, .message = std::move(msg)};
  }
};

}  // namespace cpp_serial
