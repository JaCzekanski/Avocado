#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include "utils/config_observer.h"

using json = nlohmann::json;

namespace DefaultKeyBindings {
json none();
json keyboard_numpad();
json mouse();
json controller();
}  // namespace DefaultKeyBindings

enum class RenderingMode { SOFTWARE = 0u, HARDWARE = 1u, MIXED = 2u };

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
}  // namespace ControllerType