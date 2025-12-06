#include "header.h"

int main(int argc, char **argv) {
    // 调试信息输出
    //std::cout << "Home directory: " << home_dir << std::endl;
    //std::cout << "Current directory: " << dir_now << std::endl;
    // 构造duckshell配置目录和插件列表文件路径
    std::string duckshell_dir = home_dir + "/duckshell";
    std::string plugins_file = duckshell_dir + "/plugins.ls";
    std::string plugins_dir = duckshell_dir + "/plugins";

    // 检查并创建duckshell配置目录及插件文件
    struct stat info{};
    if (stat(duckshell_dir.c_str(), &info) != 0) {
#ifdef _WIN64
        // Windows系统下创建目录
        _mkdir(duckshell_dir.c_str());
        _mkdir(plugins_dir.c_str());
#else
        // Unix/Linux系统下创建目录，设置权限为0755
        mkdir(duckshell_dir.c_str(), 0755);
        chown(plugins_dir.c_str(), 0755);
#endif
        // 创建空的插件列表文件
        std::ofstream outfile(plugins_file);
        outfile << "// duckshell plugins list" << std::endl;
        outfile << "// Do not change this file!" << std::endl;
        outfile << "{" << std::endl << std::endl;
        outfile << "}" << std::endl;
        outfile.close();
    }
    else {
        // 目录已存在，检查插件列表文件和插件目录
        if (stat(plugins_file.c_str(), &info) != 0) {
            // 插件列表文件不存在，创建空文件
            std::ofstream outfile(plugins_file);
            outfile << "// DuckShell plugins list" << std::endl;
            outfile << "// Do not change this file!" << std::endl;
            outfile << "{" << std::endl << std::endl;
            outfile << "}" << std::endl;
            outfile.close();
        }

        if (stat(plugins_dir.c_str(), &info) != 0) {
#ifdef _WIN64
            // Windows系统下创建插件目录
            _mkdir(plugins_dir.c_str());
#else
            // Unix/Linux系统下创建插件目录，设置权限为0755
            mkdir(plugins_dir.c_str(), 0755);
#endif
        }
    }


    if (argc < 2) {
        startup();
    }
    return 0;
}