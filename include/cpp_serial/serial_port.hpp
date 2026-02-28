#pragma once

#include "cpp_serial/serial_backend.hpp"

#include <memory>
#include <mutex>
#include <span>
#include <string>
#include <vector>

namespace cpp_serial {

class SerialPort {
 public:
  SerialPort();
  explicit SerialPort(std::unique_ptr<ISerialBackend> backend);
  ~SerialPort() = default;

  SerialPort(const SerialPort&) = delete;
  SerialPort& operator=(const SerialPort&) = delete;

  Status open(const std::string& portName, const SerialConfig& config);
  Status close();
  bool isOpen() const;

  Status write(std::span<const std::uint8_t> data, std::size_t& written);
  Status read(std::size_t maxBytes, std::vector<std::uint8_t>& out);

 private:
  static Status validateConfig(const SerialConfig& config);

  std::unique_ptr<ISerialBackend> backend_;
  mutable std::mutex mutex_;
};

}  // namespace cpp_serial
