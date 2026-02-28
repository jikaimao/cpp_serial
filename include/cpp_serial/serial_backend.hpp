#pragma once

#include "cpp_serial/serial_types.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace cpp_serial {

class ISerialBackend {
 public:
  virtual ~ISerialBackend() = default;

  virtual Status open(const std::string& portName, const SerialConfig& config) = 0;
  virtual Status close() = 0;
  virtual bool isOpen() const = 0;

  virtual Status write(const std::vector<std::uint8_t>& bytes, std::size_t& written) = 0;
  virtual Status read(std::size_t maxBytes, std::vector<std::uint8_t>& out) = 0;
};

std::unique_ptr<ISerialBackend> makeDefaultBackend();

}  // namespace cpp_serial
