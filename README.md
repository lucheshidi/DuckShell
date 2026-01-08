# 🦆 DuckShell (DUCKSHELL)

[![Windows Build](https://github.com/lucheshidi/DuckShell/actions/workflows/windows.yml/badge.svg)](https://github.com/lucheshidi/DuckShell/actions/workflows/windows.yml)
[![Linux Build](https://github.com/lucheshidi/DuckShell/actions/workflows/linux.yml/badge.svg)](https://github.com/lucheshidi/DuckShell/actions/workflows/linux.yml)
[![macOS Build](https://github.com/lucheshidi/DuckShell/actions/workflows/macos.yml/badge.svg)](https://github.com/lucheshidi/DuckShell/actions/workflows/macos.yml)

***English*** | [简体中文](README_zh-CN.md)

DuckShell is a lightweight, cross-platform, and highly extensible modern shell. It aims to provide developers with a simple yet feature-rich command-line environment through a powerful C++ plugin system.

## 🌟 Key Features

- **Extremely Lightweight**: The core program is only about 20 MB, starting up in an instant.
- **Truly Cross-platform**: Native support for Windows (x86_64, ARM64), Linux, and macOS.
- **Powerful Plugin System**: Supports writing custom plugins in C++ for infinite functionality expansion.
- **Automated Build**: Based on CMake FetchContent, achieving "zero manual dependency" installation by automatically handling zlib and minizip.
- **Open Source Spirit**: Fully open source, community contributions are welcome.

## 📥 Installation Guide

You can download the pre-compiled binaries for your platform from the [Latest Release](https://github.com/lucheshidi/DuckShell/releases/latest) page.

### Supported Architectures
- **Windows**: `x86_64` (MinGW), `ARM64` (Clang)
- **Linux**: `amd64`, `arm64`
- **macOS**: `Intel`, `Apple Silicon` (Universal .dmg)

## 🛠️ Compilation Instructions

DuckShell now implements automated dependency management; you don't need to manually install `zlib` or `minizip`.

### Prerequisites
- CMake (>= 3.15)
- C++17 compatible compiler (GCC, Clang, or MSYS2 MinGW)
- (Linux/macOS only) `libcurl` development library

### Build Steps
```bash
# Clone the repository
git clone https://github.com/lucheshidi/DuckShell.git
cd DuckShell

# Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## 🚀 Usage

- **Windows**: Run `DuckShell.exe` directly.
- **Unix-like**: Run `./DuckShell`.

### Common Commands
- `help`: Display help information.
- `cls` / `clear`: Clear the screen.
- `plugin`: Enter plugin management mode.

## 🔌 Plugin System

The essence of DuckShell lies in its plugins.

- **Installing Plugins**: Place compiled plugins (`.dll` / `.so` / `.dylib`) into the `~/duckshell/plugins/` directory.
- **Remote Download**: Supports automatic download and extraction of plugins from configured repositories (relying on automated minizip integration).
- **Writing Plugins**: Refer to the `ExamplePlugins/HelloWorld` example, use the interface defined in `plugins_interface.h` to get started quickly.

## 🤝 Contributing

We welcome any form of contribution!

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

---

*DuckShell - Making the command line simple again.*
