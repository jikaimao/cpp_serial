#include "cpp_serial/serial_backend.hpp"

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>

#include <algorithm>
#include <limits>
#include <sstream>
#include <string>

namespace cpp_serial {
namespace {

DWORD toParity(Parity parity) {
  switch (parity) {
    case Parity::None:
      return NOPARITY;
    case Parity::Odd:
      return ODDPARITY;
    case Parity::Even:
      return EVENPARITY;
  }
  return NOPARITY;
}

DWORD toStopBits(StopBits bits) {
  return bits == StopBits::Two ? TWOSTOPBITS : ONESTOPBIT;
}

std::string normalizePortName(const std::string& portName) {
  constexpr const char* kPrefix = "\\\\.\\";
  if (portName.rfind(kPrefix, 0) == 0) {
    return portName;
  }
  return std::string(kPrefix) + portName;
}

class WindowsSerialBackend final : public ISerialBackend {
 public:
  ~WindowsSerialBackend() override {
    if (handle_ != INVALID_HANDLE_VALUE) {
      CloseHandle(handle_);
    }
  }

  Status open(const std::string& portName, const SerialConfig& config) override {
    if (handle_ != INVALID_HANDLE_VALUE) {
      return Status::failure(ErrorCode::InvalidState, "serial handle already open");
    }

    const std::string fullName = normalizePortName(portName);
    handle_ = CreateFileA(fullName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                          OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle_ == INVALID_HANDLE_VALUE) {
      return Status::failure(ErrorCode::OpenFailed, lastError("CreateFileA failed"));
    }

    if (!SetupComm(handle_, 4096, 4096)) {
      const auto err = Status::failure(ErrorCode::OpenFailed, lastError("SetupComm failed"));
      closeIgnoreError();
      return err;
    }

    DCB dcb{};
    dcb.DCBlength = sizeof(DCB);
    if (!GetCommState(handle_, &dcb)) {
      const auto err = Status::failure(ErrorCode::OpenFailed, lastError("GetCommState failed"));
      closeIgnoreError();
      return err;
    }

    dcb.BaudRate = config.baudRate;
    dcb.ByteSize = config.dataBits;
    dcb.Parity = static_cast<BYTE>(toParity(config.parity));
    dcb.StopBits = static_cast<BYTE>(toStopBits(config.stopBits));
    dcb.fBinary = TRUE;
    dcb.fParity = config.parity != Parity::None;
    dcb.fOutxCtsFlow = config.flowControl == FlowControl::RtsCts;
    dcb.fRtsControl = config.flowControl == FlowControl::RtsCts ? RTS_CONTROL_HANDSHAKE : RTS_CONTROL_DISABLE;
    dcb.fDtrControl = DTR_CONTROL_ENABLE;

    if (!SetCommState(handle_, &dcb)) {
      const auto err = Status::failure(ErrorCode::OpenFailed, lastError("SetCommState failed"));
      closeIgnoreError();
      return err;
    }

    COMMTIMEOUTS timeouts{};
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = static_cast<DWORD>(config.readTimeout.count());
    timeouts.WriteTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = static_cast<DWORD>(config.writeTimeout.count());

    if (!SetCommTimeouts(handle_, &timeouts)) {
      const auto err = Status::failure(ErrorCode::OpenFailed, lastError("SetCommTimeouts failed"));
      closeIgnoreError();
      return err;
    }

    if (!PurgeComm(handle_, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT)) {
      const auto err = Status::failure(ErrorCode::OpenFailed, lastError("PurgeComm failed"));
      closeIgnoreError();
      return err;
    }

    return Status::success();
  }

  Status close() override {
    if (handle_ == INVALID_HANDLE_VALUE) {
      return Status::success();
    }

    if (!CloseHandle(handle_)) {
      return Status::failure(ErrorCode::CloseFailed, lastError("CloseHandle failed"));
    }

    handle_ = INVALID_HANDLE_VALUE;
    return Status::success();
  }

  bool isOpen() const override { return handle_ != INVALID_HANDLE_VALUE; }

  Status write(const std::vector<std::uint8_t>& bytes, std::size_t& written) override {
    written = 0;
    if (handle_ == INVALID_HANDLE_VALUE) {
      return Status::failure(ErrorCode::InvalidState, "write requested on closed backend");
    }
    if (bytes.empty()) {
      return Status::success();
    }

    constexpr std::size_t kMaxDword = static_cast<std::size_t>(std::numeric_limits<DWORD>::max());
    const DWORD chunk = static_cast<DWORD>(std::min(bytes.size(), kMaxDword));

    DWORD localWritten = 0;
    if (!WriteFile(handle_, bytes.data(), chunk, &localWritten, nullptr)) {
      return Status::failure(ErrorCode::WriteFailed, lastError("WriteFile failed"));
    }

    written = static_cast<std::size_t>(localWritten);
    return Status::success();
  }

  Status read(std::size_t maxBytes, std::vector<std::uint8_t>& out) override {
    out.clear();
    if (handle_ == INVALID_HANDLE_VALUE) {
      return Status::failure(ErrorCode::InvalidState, "read requested on closed backend");
    }
    if (maxBytes == 0) {
      return Status::success();
    }

    constexpr std::size_t kMaxDword = static_cast<std::size_t>(std::numeric_limits<DWORD>::max());
    const DWORD chunk = static_cast<DWORD>(std::min(maxBytes, kMaxDword));

    out.resize(chunk);
    DWORD localRead = 0;
    if (!ReadFile(handle_, out.data(), chunk, &localRead, nullptr)) {
      out.clear();
      return Status::failure(ErrorCode::ReadFailed, lastError("ReadFile failed"));
    }

    out.resize(localRead);
    return Status::success();
  }

 private:
  void closeIgnoreError() {
    if (handle_ != INVALID_HANDLE_VALUE) {
      CloseHandle(handle_);
      handle_ = INVALID_HANDLE_VALUE;
    }
  }

  static std::string lastError(const char* prefix) {
    std::ostringstream os;
    os << prefix << " (GetLastError=" << GetLastError() << ")";
    return os.str();
  }

  HANDLE handle_{INVALID_HANDLE_VALUE};
};

}  // namespace

std::unique_ptr<ISerialBackend> makeWindowsBackend() {
  return std::make_unique<WindowsSerialBackend>();
}

}  // namespace cpp_serial
#endif
