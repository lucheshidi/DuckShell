#include "plugins_interface.h"
#include <iostream>
#include <map>
#include <vector>

class HelloWorldPlugin : public IPlugin {
private:
    bool enabled;
    std::map<std::string, std::string> config;

public:
    HelloWorldPlugin() : enabled(true) {}

    // 实现新接口的方法
    bool on_setup(const PluginContext& context) override {
        std::cout << "[HelloWorldPlugin] Setting up... Home: " << context.home_dir << std::endl;
        return true;
    }

    void on_execute(const std::vector<std::string>& args) override {
        if (!enabled) {
            std::cout << "[HelloWorldPlugin] Plugin is currently disabled!" << std::endl;
            return;
        }

        std::cout << "[HelloWorldPlugin] Hello World! This is a working plugin." << std::endl;
        if (!args.empty()) {
            std::cout << "[HelloWorldPlugin] Arguments received:" << std::endl;
            for (size_t i = 0; i < args.size(); ++i) {
                std::cout << "  [" << i << "] " << args[i] << std::endl;
            }
        }
    }

    std::vector<std::string> get_command_aliases() override {
        return {"hello", "hi"};
    }

    std::string get_prompt() override {
        // 演示：当插件启用时，可以在 prompt 中添加点东西
        return "[Hello] ";
    }

    void on_event(const std::string& event, const std::map<std::string, std::string>& params) override {
        std::cout << "[HelloWorldPlugin] Received event: " << event << std::endl;
    }

    // 适配旧代码中可能调用的旧名称（如果有的话，但 IPlugin 接口已经变了，所以我们直接重写）
};

extern "C" {
    PLUGIN_EXPORT IPlugin* createPlugin() {
        return new HelloWorldPlugin();
    }

    PLUGIN_EXPORT void destroyPlugin(IPlugin* plugin) {
        delete plugin;
    }
}
