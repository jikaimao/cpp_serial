# cpp_serial

Windows 工业级串口通信 C++ 类库，采用 CMake 管理，日志使用 `spdlog`，并包含可执行的单元测试。

## 特性

- 面向对象封装：`cpp_serial::SerialPort`
- 线程安全（内部互斥保护）
- 严格参数校验（波特率、数据位、超时等）
- Windows 原生 API（`CreateFileA / SetCommState / SetCommTimeouts / ReadFile / WriteFile`）
- 可测试架构（通过 `ISerialBackend` 抽象底层，方便 Mock）
- 集成 `spdlog` 日志
- 错误码体系（`ErrorCode`）用于工业场景故障定位

## 工具链

- MinGW32 / g++ 15.1.0
- CMake >= 3.20

## 构建

```bash
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

> 在非 Windows 平台上可以编译核心逻辑与测试，但默认后端会提示不支持串口硬件访问。

## 运行测试

```bash
ctest --test-dir build --output-on-failure
```

## 核心接口示例

```cpp
#include "cpp_serial/serial_port.hpp"

cpp_serial::SerialPort port;
cpp_serial::SerialConfig cfg;
cfg.baudRate = 115200;
cfg.dataBits = 8;
cfg.parity = cpp_serial::Parity::None;
cfg.stopBits = cpp_serial::StopBits::One;

auto st = port.open("COM3", cfg);
if (!st.ok) {
  // st.code + st.message 进行故障处理
}
```

## Example

仓库提供了示例程序：`examples/basic_example.cpp`。

```bash
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build -j --target cpp_serial_example
```

运行后会尝试打开 `COM3` 并执行一次写入/读取，你可以按设备实际端口修改示例中的端口号和报文字节。
