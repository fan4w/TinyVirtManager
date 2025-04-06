#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <string>
#include <map>

class ConfigManager {
private:
    std::map<std::string, std::string> configMap;
    std::string configFilePath;

    ConfigManager() = default;
    bool parseConfigFile();

public:
    static ConfigManager* Instance();
    bool init(const std::string& configPath);
    std::string getValue(const std::string& key, const std::string& defaultValue = "") const;
    int getIntValue(const std::string& key, int defaultValue = 0) const;
};

#endif // CONFIG_MANAGER_H