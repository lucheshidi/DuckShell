### DuckShell 插件系统使用教程 (V2.0)

本文档介绍如何为 DuckShell 开发和使用插件。

#### 1. 简介
DuckShell 插件系统允许开发者通过动态链接库 (DLL/SO) 扩展 Shell 的功能。新版本的插件接口采用了下划线命名法，并增加了初始化钩子和自定义 Prompt 等功能。

#### 2. 插件接口 (`plugins_interface.h`)

所有的插件必须继承自 `IPlugin` 类并实现其虚函数。

```cpp
class IPlugin {
public:
    virtual ~IPlugin() = default;

    /**
     * @brief 插件初始化时调用
     * @param context 包含 shell 的上下文信息（如 home_dir, current_dir）
     */
    virtual bool on_setup(const PluginContext& context) { return true; }

    /**
     * @brief 执行插件主逻辑（作为命令调用时）
     * @param args 命令行参数列表
     */
    virtual void on_execute(const std::vector<std::string>& args) = 0;

    /**
     * @brief 获取插件关联的命令别名
     */
    virtual std::vector<std::string> get_command_aliases() = 0;

    /**
     * @brief 获取插件自定义的 Shell Prompt
     */
    virtual std::string get_prompt() { return ""; }

    /**
     * @brief 事件处理回调
     */
    virtual void on_event(const std::string& event, const std::map<std::string, std::string>& params) {}
};
```

#### 3. 开发步骤

1.  **包含头文件**: 在你的项目中包含 `plugins_interface.h`。
2.  **实现插件类**: 创建一个继承自 `IPlugin` 的类。
3.  **导出工厂函数**: 使用 `extern "C"` 导出 `createPlugin` 和 `destroyPlugin` 函数。

示例代码 (`main.cpp`):
```cpp
#include "plugins_interface.h"
#include <iostream>

class MyPlugin : public IPlugin {
public:
    bool on_setup(const PluginContext& context) override {
        std::cout << "插件已安装！Home 目录: " << context.home_dir << std::endl;
        return true;
    }

    void on_execute(const std::vector<std::string>& args) override {
        std::cout << "正在执行 MyPlugin..." << std::endl;
    }

    std::vector<std::string> get_command_aliases() override {
        return {"mycmd", "test"};
    }

    std::string get_prompt() override {
        return "(my-shell) ";
    }
};

extern "C" {
    PLUGIN_EXPORT IPlugin* createPlugin() {
        return new MyPlugin();
    }

    PLUGIN_EXPORT void destroyPlugin(IPlugin* plugin) {
        delete plugin;
    }
}
```

#### 4. 编译插件

使用 CMake 或编译器将代码编译为动态库。
*   Windows: `MyPlugin.dll`
*   Linux: `libMyPlugin.so`

#### 5. 安装与运行

1.  将生成的动态库放入 `~/.duckshell/plugins/` 目录。
2.  在 DuckShell 中运行 `plugin install MyPlugin` (假设文件名为 MyPlugin.dll)。
3.  使用别名执行命令，例如：`mycmd`。

#### 6. 新功能说明

*   **`on_setup`**: 允许插件在加载时进行一次性初始化。非常适合需要初始化图形界面（如 ncurses）或加载配置的插件。
*   **`get_prompt`**: 允许插件动态修改 Shell 的提示符。如果多个插件都提供了 prompt，Shell 通常会选择优先级最高的一个或进行组合。
*   **`PluginContext`**: 提供给插件关于 Shell 运行状态的信息，避免插件自己去探测环境变量。
