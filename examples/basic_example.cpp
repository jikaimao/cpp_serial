#include "cpp_serial/serial_port.hpp"
#include <chrono>
#include <thread>
#include <cstdint>
#include <iostream>
#include <vector>

int main() {
  auto start_time = std::chrono::high_resolution_clock::now();
  cpp_serial::SerialPort port;

  cpp_serial::SerialConfig cfg;
  cfg.baudRate = 115200;
  cfg.dataBits = 8;
  cfg.parity = cpp_serial::Parity::None;
  cfg.stopBits = cpp_serial::StopBits::One;
  cfg.flowControl = cpp_serial::FlowControl::None;
  cfg.readTimeout = std::chrono::milliseconds(50);
  cfg.writeTimeout = std::chrono::milliseconds(50);

  const auto openStatus = port.open("COM20", cfg);
  if (!openStatus.ok) {
    std::cerr << "Open failed, code=" << static_cast<int>(openStatus.code)
              << ", message=" << openStatus.message << '\n';
    return 1;
  }

  const std::vector<std::uint8_t> tx{0x01, 0x03, 0x0c, 0x00};
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
  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
  std::cout << std::dec << "Total execution time: " << duration.count() << " us (" 
            << duration.count() / 1000.0 << " ms)\n";
  return 0;
}
