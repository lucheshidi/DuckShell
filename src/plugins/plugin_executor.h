// plugins/plugin_executor.h
#ifndef PLUGIN_EXECUTOR_H
#define PLUGIN_EXECUTOR_H

#include <string>
#include <vector>
#include <map>

class PluginExecutor {
public:
    static void execute_command(const std::string& command, const std::vector<std::string>& cmd_args);
    static void execute_plugin_with_command(const std::string& plugin_name, const std::vector<std::string>& cmd_args);
};

#endif // PLUGIN_EXECUTOR_H