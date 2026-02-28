#include "cpp_serial/serial_port.hpp"

#include <spdlog/spdlog.h>

#include <string>
#include <vector>

namespace cpp_serial {

SerialPort::SerialPort() : backend_(makeDefaultBackend()) {}

SerialPort::SerialPort(std::unique_ptr<ISerialBackend> backend)
    : backend_(backend ? std::move(backend) : makeDefaultBackend()) {}

Status SerialPort::open(const std::string& portName, const SerialConfig& config) {
  std::lock_guard lock(mutex_);
  const auto check = validateConfig(config);
  if (!check.ok) {
    spdlog::error("Serial config invalid for {}: {}", portName, check.message);
    return check;
  }

  if (backend_->isOpen()) {
    const std::string msg = "port already open";
    spdlog::warn("{}: {}", portName, msg);
    return Status::failure(ErrorCode::InvalidState, msg);
  }

  auto result = backend_->open(portName, config);
  if (!result.ok) {
    spdlog::error("Failed to open {}: {}", portName, result.message);
    return result;
  }

  spdlog::info("Opened serial port {} at {} bps", portName, config.baudRate);
  return Status::success();
}

Status SerialPort::close() {
  std::lock_guard lock(mutex_);
  if (!backend_->isOpen()) {
    return Status::success();
  }

  auto result = backend_->close();
  if (!result.ok) {
    spdlog::error("Failed to close serial port: {}", result.message);
    return result;
  }

  spdlog::info("Serial port closed");
  return Status::success();
}

bool SerialPort::isOpen() const {
  std::lock_guard lock(mutex_);
  return backend_->isOpen();
}

Status SerialPort::write(std::span<const std::uint8_t> data, std::size_t& written) {
  std::lock_guard lock(mutex_);
  if (!backend_->isOpen()) {
    return Status::failure(ErrorCode::InvalidState, "write requested on closed port");
  }

  std::vector<std::uint8_t> payload(data.begin(), data.end());
  auto result = backend_->write(payload, written);
  if (!result.ok) {
    spdlog::error("Serial write failed: {}", result.message);
  }
  return result;
}

Status SerialPort::read(std::size_t maxBytes, std::vector<std::uint8_t>& out) {
  std::lock_guard lock(mutex_);
  if (!backend_->isOpen()) {
    return Status::failure(ErrorCode::InvalidState, "read requested on closed port");
  }

  auto result = backend_->read(maxBytes, out);
  if (!result.ok) {
    spdlog::error("Serial read failed: {}", result.message);
  }
  return result;
}

Status SerialPort::validateConfig(const SerialConfig& config) {
  if (config.baudRate == 0) {
    return Status::failure(ErrorCode::InvalidArgument, "baudRate must be > 0");
  }

  if (config.dataBits < 5 || config.dataBits > 8) {
    return Status::failure(ErrorCode::InvalidArgument, "dataBits must be within [5,8]");
  }

  if (config.readTimeout.count() < 0 || config.writeTimeout.count() < 0) {
    return Status::failure(ErrorCode::InvalidArgument, "timeouts must be >= 0");
  }

  return Status::success();
}

}  // namespace cpp_serial
