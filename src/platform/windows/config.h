#pragma once
#include <json.hpp>

using json = nlohmann::json;

extern const char* CONFIG_NAME;
extern const nlohmann::json defaultConfig;
extern nlohmann::json config;
void saveConfigFile(const char* configName);
void loadConfigFile(const char* configName);

bool isEmulatorConfigured();