// plugins/pluginS_interface.h
#ifndef PLUGINS_INTERFACE_H
#define PLUGINS_INTERFACE_H

#include <string>
#include <vector>
#include <map>

class IPlugin {
public:
    virtual ~IPlugin() = default;

    // 基础接口
    virtual std::string getName() const = 0;
    virtual std::string getVersion() const = 0;
    virtual std::string getDescription() const = 0;
    virtual void execute() = 0;

    // 生命周期管理
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;

    // 配置管理
    virtual bool setConfig(const std::string& key, const std::string& value) = 0;
    virtual std::string getConfig(const std::string& key) const = 0;

    // 状态管理
    virtual bool isEnabled() const = 0;
    virtual void setEnabled(bool enabled) = 0;

    // 依赖管理
    virtual std::vector<std::string> getDependencies() const = 0;

    // 事件处理
    virtual void onEvent(const std::string& eventName, const std::map<std::string, std::string>& params) = 0;

    // 错误处理
    virtual std::string getLastError() const = 0;
};

// 插件工厂函数指针类型
typedef IPlugin* (*CreatePluginFunc)();
typedef void (*DestroyPluginFunc)(IPlugin*);

// 导出符号定义
#ifdef _WIN32
    #define PLUGIN_EXPORT __declspec(dllexport)
#else
    #define PLUGIN_EXPORT __attribute__((visibility("default")))
#endif

// 插件API版本
#define PLUGIN_API_VERSION "1.0"

#endif // PLUGINS_INTERFACE_H