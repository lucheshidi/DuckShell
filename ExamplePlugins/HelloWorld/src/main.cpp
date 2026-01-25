#include "plugins_interface.h"
#include <iostream>
#include <map>

class HelloWorldPlugin : public IPlugin {
private:
    bool enabled;
    std::map<std::string, std::string> config;
    std::string lastError;

public:
    HelloWorldPlugin() : enabled(true) {}

    std::string getName() const override {
        return "HelloWorldPlugin";
    }

    std::string getVersion() const override {
        return "1.0.0";
    }

    std::string getDescription() const override {
        return "A simple hello world demonstration plugin";
    }

    void execute() override {
        if (!enabled) {
            std::cout << "[HelloWorldPlugin] Plugin is currently disabled!" << std::endl;
            return;
        }

        std::cout << "[HelloWorldPlugin] Hello World! This is a working plugin." << std::endl;
        std::cout << "[HelloWorldPlugin] Current configuration has " << config.size() << " entries." << std::endl;
    }

    bool initialize() override {
        std::cout << "[HelloWorldPlugin] Initializing plugin..." << std::endl;
        enabled = true;
        return true;
    }

    void shutdown() override {
        std::cout << "[HelloWorldPlugin] Shutting down plugin..." << std::endl;
        enabled = false;
    }

    bool setConfig(const std::string &key, const std::string &value) override {
        config[key] = value;
        return true;
    }

    std::string getConfig(const std::string &key) const override {
        auto it = config.find(key);
        if (it != config.end()) {
            return it->second;
        }
        return "";
    }

    bool isEnabled() const override {
        return enabled;
    }

    void setEnabled(bool enabled) override {
        this->enabled = enabled;
    }

    std::vector<std::string> getDependencies() const override {
        return {};
    }

    // 在 HelloWorldPlugin 的 onEvent 方法中添加处理逻辑
    void onEvent(const std::string &eventName, const std::map<std::string, std::string> &params) override {
        if (eventName == "command_execute") {
            std::cout << "[HelloWorldPlugin] Received command with " << params.at("argc") << " arguments:" << std::endl;
            int argc = std::stoi(params.at("argc"));
            for (int i = 0; i < argc; ++i) {
                std::string argKey = "arg" + std::to_string(i);
                auto it = params.find(argKey);
                if (it != params.end()) {
                    std::cout << "  Argument " << i << ": " << it->second << std::endl;
                }
            }
        }
    }


    std::string getLastError() const override {
        return lastError;
    }
};

extern "C" {
    PLUGIN_EXPORT IPlugin* createPlugin() {
        return new HelloWorldPlugin();
    }

    PLUGIN_EXPORT void destroyPlugin(IPlugin* plugin) {
        delete plugin;
    }
}
