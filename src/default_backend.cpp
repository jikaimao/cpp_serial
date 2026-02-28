#include "cpp_serial/serial_backend.hpp"

#include <memory>

namespace cpp_serial {

class UnsupportedBackend final : public ISerialBackend {
 public:
  Status open(const std::string&, const SerialConfig&) override {
    return Status::failure(ErrorCode::Unsupported, "Default backend is only available on Windows");
  }

  Status close() override { return Status::success(); }

  bool isOpen() const override { return false; }

  Status write(const std::vector<std::uint8_t>&, std::size_t& written) override {
    written = 0;
    return Status::failure(ErrorCode::Unsupported, "Serial write unsupported on this platform");
  }

  Status read(std::size_t, std::vector<std::uint8_t>& out) override {
    out.clear();
    return Status::failure(ErrorCode::Unsupported, "Serial read unsupported on this platform");
  }
};

#ifdef _WIN32
std::unique_ptr<ISerialBackend> makeWindowsBackend();
#endif

std::unique_ptr<ISerialBackend> makeDefaultBackend() {
#ifdef _WIN32
  return makeWindowsBackend();
#else
  return std::make_unique<UnsupportedBackend>();
#endif
}

}  // namespace cpp_serial
