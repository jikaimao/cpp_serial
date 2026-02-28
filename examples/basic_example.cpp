#include "cpp_serial/serial_port.hpp"

#include <cstdint>
#include <iostream>
#include <vector>

int main() {
  cpp_serial::SerialPort port;

  cpp_serial::SerialConfig cfg;
  cfg.baudRate = 115200;
  cfg.dataBits = 8;
  cfg.parity = cpp_serial::Parity::None;
  cfg.stopBits = cpp_serial::StopBits::One;
  cfg.flowControl = cpp_serial::FlowControl::None;
  cfg.readTimeout = std::chrono::milliseconds(200);
  cfg.writeTimeout = std::chrono::milliseconds(200);

  const auto openStatus = port.open("COM3", cfg);
  if (!openStatus.ok) {
    std::cerr << "Open failed, code=" << static_cast<int>(openStatus.code)
              << ", message=" << openStatus.message << '\n';
    return 1;
  }

  const std::vector<std::uint8_t> tx{0x01, 0x03, 0x00, 0x00, 0x00, 0x02, 0xC4, 0x0B};
  std::size_t written = 0;
  const auto writeStatus = port.write(tx, written);
  if (!writeStatus.ok) {
    std::cerr << "Write failed, code=" << static_cast<int>(writeStatus.code)
              << ", message=" << writeStatus.message << '\n';
    (void)port.close();
    return 2;
  }

  std::cout << "Write bytes: " << written << '\n';

  std::vector<std::uint8_t> rx;
  const auto readStatus = port.read(256, rx);
  if (!readStatus.ok) {
    std::cerr << "Read failed, code=" << static_cast<int>(readStatus.code)
              << ", message=" << readStatus.message << '\n';
    (void)port.close();
    return 3;
  }

  std::cout << "Read bytes: " << rx.size() << '\n';

  const auto closeStatus = port.close();
  if (!closeStatus.ok) {
    std::cerr << "Close failed, code=" << static_cast<int>(closeStatus.code)
              << ", message=" << closeStatus.message << '\n';
    return 4;
  }

  return 0;
}
