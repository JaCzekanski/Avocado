#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include "utils/config_observer.h"

#ifdef ANDROID
#include <android/log.h>
#define printf(...) __android_log_print(ANDROID_LOG_DEBUG, "AVOCADO", __VA_ARGS__);
#endif

using json = nlohmann::json;

namespace DefaultKeyBindings {
json none();
json keyboard_numpad();
json mouse();
json controller();
}  // namespace DefaultKeyBindings

enum RenderingMode { SOFTWARE = 1 << 0, HARDWARE = 1 << 1, MIXED = SOFTWARE | HARDWARE };
NLOHMANN_JSON_SERIALIZE_ENUM(RenderingMode, {
                                                {RenderingMode::SOFTWARE, "software"},
                                                {RenderingMode::HARDWARE, "hardware"},
                                                {RenderingMode::MIXED, "mixed"},
                                            });

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