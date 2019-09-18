#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include "device/controller/controller_type.h"
#include "device/gpu/rendering_mode.h"
#include "utils/event.h"

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

extern const char* CONFIG_NAME;
extern const nlohmann::json defaultConfig;
extern nlohmann::json config;
void saveConfigFile(const char* configName);
void loadConfigFile(const char* configName);

bool isEmulatorConfigured();