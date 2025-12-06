// plugins/pluginS_interface.h
#ifndef PLUGINS_INTERFACE_H
#define PLUGINS_INTERFACE_H

#include <string>

class IPlugin {
public:
    virtual ~IPlugin() = default;
    virtual std::string getName() const = 0;
    virtual void execute() = 0;
};

// 插件工厂函数指针类型
typedef IPlugin* (*CreatePluginFunc)();
typedef void (*DestroyPluginFunc)(IPlugin*);

#endif // PLUGINS_INTERFACE_H
