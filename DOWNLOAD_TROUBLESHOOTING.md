# DuckShell 插件下载功能诊断与使用指南

## 问题诊断

如果你遇到了插件下载问题，请按照以下步骤进行诊断：

### 1. 网络连接测试

首先确认你的网络连接正常：
```bash
# 在DuckShell中测试基本网络功能
plugin repo https://httpbin.org
plugin available
```

### 2. 常见问题及解决方案

#### 问题1：CURL初始化失败
- **症状**：显示"CURL初始化失败"
- **解决方案**：确保系统安装了curl库，或者重新编译项目

#### 问题2：HTTPS证书验证失败
- **症状**：SSL连接错误
- **解决方案**：
  - Windows: 确保系统已更新到最新版本
  - Linux: 安装ca-certificates包
  - `sudo apt-get install ca-certificates` (Ubuntu/Debian)

#### 问题3：schannel解密调试信息
- **症状**：显示大量`schannel: failed to decrypt data, need more data`
- **说明**：这是Windows系统的正常调试信息，不影响功能
- **解决方案**：这些信息已被过滤，不会影响正常使用

#### 问题4：跨平台编译错误
- **症状**：macOS显示`unlink`未声明，Linux ARM显示curl函数未声明
- **说明**：不同平台需要不同的头文件包含
- **解决方案**：已修复头文件包含问题，重新编译即可

#### 问题5：Linux ARM缺少curl开发包
- **症状**：`fatal error: curl/curl.h: No such file or directory`
- **说明**：系统缺少libcurl开发包
- **解决方案**：
  - Ubuntu/Debian: `sudo apt-get install libcurl4-openssl-dev`
  - CentOS/RHEL: `sudo yum install libcurl-devel`
  - 或者禁用远程仓库功能：`cmake -DENABLE_REMOTE_REPO=OFF ..`

#### 问题3：仓库URL不可访问
- **症状**：连接超时或404错误
- **解决方案**：
  ```bash
  # 设置可用的测试仓库
  plugin repo https://httpbin.org
  ```

#### 问题4：文件权限问题
- **症状**：无法创建文件
- **解决方案**：
  - 确保`~/duckshell/`目录存在且有写权限
  - Windows: 检查防病毒软件是否阻止文件创建

### 3. 测试下载功能

使用以下命令测试下载功能：

```bash
# 1. 检查当前仓库设置
plugin repo

# 2. 列出可用插件（测试连接）
plugin available

# 3. 下载测试插件
plugin download test-plugin

# 4. 查看已安装插件
plugin list
```

## 新功能特性

### 增强的错误处理
- 详细的下载进度显示
- 具体的错误代码和描述
- 连接超时和重试机制

### 改进的用户体验
- 更清晰的命令提示
- 自动创建必要的目录
- 友好的错误消息

## 故障排除命令

```bash
# 查看详细错误信息
plugin download <plugin_name>  # 会显示详细的下载过程

# 测试基本网络连接
plugin repo https://httpbin.org
plugin available

# 检查本地插件状态
plugin list

# 重新设置仓库URL
plugin repo <new_repository_url>
```

## 开发者测试

如果你想要深入测试下载功能，可以使用提供的测试程序：

1. **编译测试程序**：
```bash
g++ -o simple_download_test simple_download_test.cpp -lcurl
./simple_download_test
```

2. **检查构建配置**：
确保CMakeLists.txt中启用了远程仓库功能：
```cmake
option(ENABLE_REMOTE_REPO "Enable remote plugin repository features" ON)
```

## 常见仓库URL格式

插件仓库应遵循以下结构：
```
repository_root/
├── simple/
│   └── index.html          # 插件索引页面
└── packages/
    └── plugin_name/
        ├── plugin.json     # 插件元数据
        └── v1.0/
            ├── plugin.dll  # Windows插件
            └── plugin.so   # Linux插件
```

## 联系支持

如果以上方法都无法解决问题，请提供以下信息：
1. 操作系统版本
2. 完整的错误消息
3. 网络环境信息
4. 测试结果截图