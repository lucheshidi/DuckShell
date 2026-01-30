#!/bin/bash
# 构建测试脚本 - 验证所有平台的编译

echo "=== DuckShell Multi-Platform Build Test ==="

# 测试目录
TEST_DIR="/tmp/duckshell_build_test"
mkdir -p "$TEST_DIR"
cd "$TEST_DIR"

# 克隆仓库（假设在当前目录有源码）
echo "Copying source files..."
cp -r /path/to/duckshell/source/* .

# 测试各个平台配置

echo -e "\n1. Testing Linux x86_64 build..."
mkdir -p build_linux_x86
cd build_linux_x86
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_SYSTEM_NAME=Linux -DCMAKE_SYSTEM_PROCESSOR=x86_64
if make -j$(nproc) 2>&1 | grep -q "error"; then
    echo "❌ Linux x86_64 build FAILED"
    exit 1
else
    echo "✅ Linux x86_64 build SUCCESS"
fi
cd ..

echo -e "\n2. Testing Linux ARM64 build..."
mkdir -p build_linux_arm
cd build_linux_arm
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_SYSTEM_NAME=Linux -DCMAKE_SYSTEM_PROCESSOR=aarch64
if make -j$(nproc) 2>&1 | grep -q "error"; then
    echo "❌ Linux ARM64 build FAILED"
    exit 1
else
    echo "✅ Linux ARM64 build SUCCESS"
fi
cd ..

echo -e "\n3. Testing macOS build..."
mkdir -p build_macos
cd build_macos
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_SYSTEM_NAME=Darwin
if make -j$(nproc) 2>&1 | grep -q "error"; then
    echo "❌ macOS build FAILED"
    exit 1
else
    echo "✅ macOS build SUCCESS"
fi
cd ..

echo -e "\n4. Testing Windows build..."
mkdir -p build_windows
cd build_windows
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_SYSTEM_NAME=Windows
if make -j$(nproc) 2>&1 | grep -q "error"; then
    echo "❌ Windows build FAILED"
    exit 1
else
    echo "✅ Windows build SUCCESS"
fi
cd ..

echo -e "\n=== All builds completed successfully! ==="
rm -rf "$TEST_DIR"