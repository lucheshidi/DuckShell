// plugins/pluginS_interface.h
#ifndef PLUGINS_INTERFACE_H
#define PLUGINS_INTERFACE_H

#include <string>
#include <vector>
#include <map>

struct IPlugin {
    virtual ~IPlugin() = default;
    virtual void execute() = 0;
    virtual std::vector<std::string> getCommandAliases() = 0;
    virtual void onEvent(const std::string& event, const std::map<std::string, std::string>& params) = 0;
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