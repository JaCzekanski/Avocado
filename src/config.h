#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include "utils/config_observer.h"

using json = nlohmann::json;

extern const char* CONFIG_NAME;
extern const nlohmann::json defaultConfig;
extern nlohmann::json config;
void saveConfigFile(const char* configName);
void loadConfigFile(const char* configName);

bool isEmulatorConfigured();

extern ConfigObserver configObserver;

namespace ControllerType {
    extern const std::string NONE;
    extern const std::string DIGITAL;
    extern const std::string ANALOG;
    extern const std::string MOUSE;
}