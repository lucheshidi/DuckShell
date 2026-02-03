// plugins/plugins_interface.h
#ifndef PLUGINS_INTERFACE_H
#define PLUGINS_INTERFACE_H

#include <string>
#include <vector>
#include <map>
#include <memory>

/**
 * @brief 插件上下文信息，传递给插件使用
 */
struct PluginContext {
    std::string home_dir;
    std::string current_dir;
    std::map<std::string, std::string>* global_vars;
    // 可以根据需要添加更多 shell 暴露给插件的接口或数据
};

/**
 * @brief 插件接口类
 */
class IPlugin {
public:
    virtual ~IPlugin() = default;

    /**
     * @brief 插件初始化/安装时调用。例如用于初始化 UI (ncurses)
     * @return bool 初始化是否成功
     */
    virtual bool on_setup(const PluginContext& context) { return true; }

    /**
     * @brief 执行插件主逻辑（当作为命令调用时）
     * @param args 命令行参数
     */
    virtual void on_execute(const std::vector<std::string>& args) = 0;

    /**
     * @brief 获取插件关联的命令别名
     * @return std::vector<std::string> 别名列表
     */
    virtual std::vector<std::string> get_command_aliases() = 0;

    /**
     * @brief 获取插件自定义的 Shell Prompt。如果返回空字符串，则使用系统默认。
     * @return std::string 自定义的 Prompt 字符串
     */
    virtual std::string get_prompt() { return ""; }

    /**
     * @brief 事件处理回调
     * @param event 事件名称
     * @param params 事件参数
     */
    virtual void on_event(const std::string& event, const std::map<std::string, std::string>& params) {}

    // 兼容性保留（可选）
    virtual void execute() { on_execute({}); }
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

// 插件 API 版本
#define PLUGIN_API_VERSION "2.0"

#endif // PLUGINS_INTERFACE_H