#include "cpp_serial/serial_port.hpp"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace cpp_serial {
namespace {

class FakeBackend final : public ISerialBackend {
 public:
  Status open(const std::string&, const SerialConfig&) override {
    if (failOpen) {
      return Status::failure(ErrorCode::OpenFailed, "forced open failure");
    }
    openState = true;
    return Status::success();
  }

  Status close() override {
    openState = false;
    return Status::success();
  }

  bool isOpen() const override { return openState; }

  Status write(const std::vector<std::uint8_t>& bytes, std::size_t& written) override {
    written = bytes.size();
    lastWrite = bytes;
    return Status::success();
  }

  Status read(std::size_t maxBytes, std::vector<std::uint8_t>& out) override {
    out.assign(readBuffer.begin(), readBuffer.begin() + std::min(maxBytes, readBuffer.size()));
    return Status::success();
  }

  bool failOpen{false};
  bool openState{false};
  std::vector<std::uint8_t> lastWrite{};
  std::vector<std::uint8_t> readBuffer{0x01, 0x02, 0x03};
};

int g_failures = 0;

void check(bool cond, const std::string& msg) {
  if (!cond) {
    ++g_failures;
    std::cerr << "[FAILED] " << msg << '\n';
  }
}

void testOpenRejectsInvalidConfig() {
  auto backend = std::make_unique<FakeBackend>();
  auto* raw = backend.get();
  SerialPort port(std::move(backend));

  SerialConfig cfg;
  cfg.dataBits = 4;

  const auto status = port.open("COM1", cfg);
  check(!status.ok, "invalid config should fail open");
  check(status.code == ErrorCode::InvalidArgument, "invalid config should report InvalidArgument");
  check(!raw->openState, "backend must stay closed");
}

void testOpenCloseHappyPath() {
  SerialPort port(std::make_unique<FakeBackend>());

  const auto openStatus = port.open("COM1", SerialConfig{});
  check(openStatus.ok, "open should succeed");
  check(port.isOpen(), "port should be open");

  const auto closeStatus = port.close();
  check(closeStatus.ok, "close should succeed");
  check(!port.isOpen(), "port should be closed");
}

void testReadWriteRequiresOpen() {
  SerialPort port(std::make_unique<FakeBackend>());

  std::size_t written = 0;
  const std::vector<std::uint8_t> payload{0x11, 0x22};
  const auto writeStatus = port.write(payload, written);
  check(!writeStatus.ok, "write should fail when closed");
  check(writeStatus.code == ErrorCode::InvalidState, "closed write should be InvalidState");

  std::vector<std::uint8_t> out;
  const auto readStatus = port.read(2, out);
  check(!readStatus.ok, "read should fail when closed");
  check(readStatus.code == ErrorCode::InvalidState, "closed read should be InvalidState");
}

void testReadWriteAfterOpenWorks() {
  auto backend = std::make_unique<FakeBackend>();
  auto* raw = backend.get();
  SerialPort port(std::move(backend));

  check(port.open("COM1", SerialConfig{}).ok, "open should succeed before read/write");

  std::size_t written = 0;
  const std::vector<std::uint8_t> payload{0x11, 0x22};
  check(port.write(payload, written).ok, "write should succeed");
  check(written == 2u, "written size should be 2");
  check(raw->lastWrite.size() == 2u, "fake backend should capture payload");
  check(raw->lastWrite[0] == 0x11, "payload byte[0] should match");

  std::vector<std::uint8_t> out;
  check(port.read(2, out).ok, "read should succeed");
  check(out.size() == 2u, "read size should be 2");
  check(out[0] == 0x01 && out[1] == 0x02, "read content should match fake data");
}

void testDoubleOpenRejected() {
  SerialPort port(std::make_unique<FakeBackend>());
  check(port.open("COM1", SerialConfig{}).ok, "first open should succeed");

  const auto status = port.open("COM1", SerialConfig{});
  check(!status.ok, "second open should fail");
  check(status.code == ErrorCode::InvalidState, "second open should be InvalidState");
}

}  // namespace
}  // namespace cpp_serial

int main() {
  cpp_serial::testOpenRejectsInvalidConfig();
  cpp_serial::testOpenCloseHappyPath();
  cpp_serial::testReadWriteRequiresOpen();
  cpp_serial::testReadWriteAfterOpenWorks();
  cpp_serial::testDoubleOpenRejected();

  if (cpp_serial::g_failures == 0) {
    std::cout << "All serial port tests passed.\n";
    return 0;
  }

  std::cerr << cpp_serial::g_failures << " test(s) failed.\n";
  return 1;
}
