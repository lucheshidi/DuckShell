// plugins/plugin_repository.h
#ifndef PLUGIN_REPOSITORY_H
#define PLUGIN_REPOSITORY_H

#include <string>

class PluginRepository {
public:
    static void set_repository_url(const std::string& url);
    static void list_remote_plugins();
    static void download_plugin(const std::string& plugin_name);

private:
    static bool internal_download(const std::string& url, const std::string& local_path);
};

#endif // PLUGIN_REPOSITORY_H