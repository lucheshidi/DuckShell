#include "plugin_installer.h"
#include "plugin_loader.h"
#include "../global_vars.h"
#include <iostream>
#include <fstream>
#include <set>
#include <sstream>
#include <sys/stat.h>
#include "plugins_interface.h"
#include "../header.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#include <dlfcn.h>      // Unix 动态库加载
#include <unistd.h>     // 包含 unlink 函数
#endif

void PluginInstaller::install_plugin(const std::string& plugin_name) {
    std::string plugin_path = home_dir + "/duckshell/plugins/" + plugin_name;
    struct stat buffer;

    if (stat(plugin_path.c_str(), &buffer) == 0) {
        PluginLoader::installed_plugins()[plugin_name] = true;

        // 更新插件列表文件
        std::string plugins_list_file = home_dir + "/duckshell/plugins.ls";
        std::ofstream outfile(plugins_list_file, std::ios::app);
        if (outfile.is_open()) {
            outfile << plugin_name << ":enabled\n";
            outfile.close();
        }

        println("Plugin '" << plugin_name.c_str() << "' installed successfully.\n");
    }
    else {
        println("Plugin '" << plugin_name.c_str() << "' not found in repository.\n");
    }
}

void PluginInstaller::uninstall_plugin(const std::string& plugin_name) {
    // 解析插件名称，支持不带扩展名的输入
    std::string resolved_name = PluginLoader::resolve_plugin_name(plugin_name);
    
    auto it = PluginLoader::installed_plugins().find(resolved_name);
    if (it != PluginLoader::installed_plugins().end()) {
        PluginLoader::installed_plugins().erase(it);

        // 重新写入插件列表文件（移除已卸载的插件）
        std::string plugins_list_file = home_dir + "/duckshell/plugins.ls";
        std::string temp_file = plugins_list_file + ".tmp";

        std::ifstream infile(plugins_list_file);
        std::ofstream outfile(temp_file);

        if (infile.is_open() && outfile.is_open()) {
            std::string line;
            while (std::getline(infile, line)) {
                // 修复删除逻辑：检查行是否以插件名开头且后面紧跟冒号
                if (line.substr(0, resolved_name.length()) != resolved_name ||
                    (line.length() > resolved_name.length() && line[resolved_name.length()] != ':')) {
                    outfile << line << "\n";
                }
            }
            infile.close();
            outfile.close();

            // 原子性替换文件
            if (remove(plugins_list_file.c_str()) != 0) {
                println("Warning: Failed to remove old plugins list file.\n");
            }
            if (rename(temp_file.c_str(), plugins_list_file.c_str()) != 0) {
                println("Error: Failed to rename temporary file.\n");
            }
        } else {
            println("Error: Failed to open plugins list files for update.\n");
        }

        println("Plugin '" << resolved_name.c_str() << "' uninstalled successfully.\n");
    } else {
        println("Plugin '" << resolved_name.c_str() << "' is not installed.\n");
    }
}

void PluginInstaller::remove_plugin(const std::string& plugin_name) {
    // 解析插件名称，支持不带扩展名的输入
    std::string resolved_name = PluginLoader::resolve_plugin_name(plugin_name);
    
    // 首先卸载插件（从内存和列表文件中移除）
    uninstall_plugin(resolved_name);
    
    // 然后删除插件文件
    std::string plugin_path = home_dir + "/duckshell/plugins/" + resolved_name;
    
#ifdef _WIN32
    if (DeleteFileA(plugin_path.c_str()) != 0) {
        println("Plugin file '" << resolved_name.c_str() << "' removed successfully.\n");
    } else {
        println("Failed to remove plugin file '" << resolved_name.c_str() << "'.\n");
    }
#else
    if (unlink(plugin_path.c_str()) == 0) {
        println("Plugin file '" << resolved_name.c_str() << "' removed successfully.\n");
    } else {
        println("Failed to remove plugin file '" << resolved_name.c_str() << "'.\n");
    }
#endif
}

void PluginInstaller::enable_plugin(const std::string& plugin_name) {
    // 解析插件名称，支持不带扩展名的输入
    std::string resolved_name = PluginLoader::resolve_plugin_name(plugin_name);
    
    auto it = PluginLoader::installed_plugins().find(resolved_name);
    if (it != PluginLoader::installed_plugins().end()) {
        if (it->second) {
            println("Plugin '" << resolved_name.c_str() << "' is already enabled.\n");
        } else {
            PluginLoader::installed_plugins()[resolved_name] = true;
            
            // 更新插件列表文件
            update_plugins_list_file();
            
            println("Plugin '" << resolved_name.c_str() << "' enabled successfully.\n");
        }
    } else {
        println("Plugin '" << resolved_name.c_str() << "' is not installed.\n");
    }
}

void PluginInstaller::disable_plugin(const std::string& plugin_name) {
    // 解析插件名称，支持不带扩展名的输入
    std::string resolved_name = PluginLoader::resolve_plugin_name(plugin_name);
    
    auto it = PluginLoader::installed_plugins().find(resolved_name);
    if (it != PluginLoader::installed_plugins().end()) {
        if (!it->second) {
            println("Plugin '" << resolved_name.c_str() << "' is already disabled.\n");
        } else {
            PluginLoader::installed_plugins()[resolved_name] = false;
            
            // 更新插件列表文件
            update_plugins_list_file();
            
            println("Plugin '" << resolved_name.c_str() << "' disabled successfully.\n");
        }
    } else {
        println("Plugin '" << resolved_name.c_str() << "' is not installed.\n");
    }
}

void PluginInstaller::list_available_plugins() {
    std::string plugins_dir = home_dir + "/duckshell/plugins";
    std::vector<std::string> contents = PluginLoader::get_directory_contents(plugins_dir);

    if (contents.empty()) {
        println("No available plugins found.\n");
        return;
    }

    println("Available plugins:\n");
    for (const auto& item : contents) {
        std::cout << "  " << item << std::endl;
    }
}

void PluginInstaller::list_installed_plugins() {
    if (PluginLoader::installed_plugins().empty()) {
        println("No plugins installed.\n");
        return;
    }

    println("Installed plugins:\n");
    for (const auto& pair : PluginLoader::installed_plugins()) {
        std::cout << "  " << pair.first << ": " << (pair.second ? "enabled" : "disabled") << std::endl;
    }
}

void PluginInstaller::install_all_plugins() {
    std::string plugins_dir = home_dir + "/duckshell/plugins";
    std::vector<std::string> contents = PluginLoader::get_directory_contents(plugins_dir);

    if (contents.empty()) {
        println("No plugins found to install.");
        return;
    }

    println("Scanning for plugins to install...");

    // 记录初始插件数量
    size_t initial_plugin_count = PluginLoader::installed_plugins().size();
    bool has_new_plugins = false;

    for (const auto& item : contents) {
        // 检查是否为插件文件（根据扩展名判断）
        if (item.find(".dll") != std::string::npos ||
            item.find(".so") != std::string::npos) {
            // 检查插件是否已经安装
            auto it = PluginLoader::installed_plugins().find(item);
            if (it == PluginLoader::installed_plugins().end()) {
                // 只有未安装的插件才添加
                PluginLoader::installed_plugins()[item] = true;
                println("Plugin '" << item.c_str() << "' added for installation.");
                has_new_plugins = true;
            } else {
                println("Plugin '"  << item.c_str() << "' already installed.");
            }
        }
    }

    // 只有在有新插件添加时才重写文件
    if (has_new_plugins) {
        // 重新写入整个插件列表文件，而不是追加
        std::string plugins_list_file = home_dir + "/duckshell/plugins.ls";
        std::ofstream outfile(plugins_list_file);
        if (outfile.is_open()) {
            for (const auto& pair : PluginLoader::installed_plugins()) {
                outfile << pair.first << ":" << (pair.second ? "enabled" : "disabled") << "\n";
            }
            outfile.close();
            println("Plugins list updated successfully.");
        }
    } else {
        println("No new plugins to install.");
    }
}

// 辅助函数：更新插件列表文件
void PluginInstaller::update_plugins_list_file() {
    std::string plugins_list_file = home_dir + "/duckshell/plugins.ls";
    std::ofstream outfile(plugins_list_file);
    
    if (outfile.is_open()) {
        for (const auto& pair : PluginLoader::installed_plugins()) {
            outfile << pair.first << ":" << (pair.second ? "enabled" : "disabled") << "\n";
        }
        outfile.close();
    }
}