#pragma once
#include <json.hpp>

extern const char* CONFIG_NAME;
extern nlohmann::json config;
void saveConfigFile(const char* configName);
void loadConfigFile(const char* configName);
