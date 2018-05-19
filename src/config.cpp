#include "config.h"
#include "utils/file.h"

const char* CONFIG_NAME = "config.json";

// clang-format off
const json defaultConfig = {
	{"bios", ""}, 
	{"extension", ""}, 
	{"iso", ""},
        {"controller", {
                {"up",      "Up"},
                {"right",   "Right"},
                {"down",    "Down"},
                {"left",    "Left"},
                {"triangle","Keypad 8"},
                {"circle",  "Keypad 6"},
                {"cross",   "Keypad 2"},
                {"square",  "Keypad 4"},
                {"l1",      "Keypad 7"},
                {"r1",      "Keypad 9"},
                {"l2",      "Keypad /"},
                {"r2",      "Keypad *"},
                {"select",  "Right Shift"},
                {"start",   "Return"},
        }},
        {"options", {
            {"graphics", {
                {"filtering", false}
            }}
        }},
        {"debug", {
            {"log", {
                { "system",1u },
                { "bios",  0u },
                { "cdrom", 0u }
            }}
        }}
};
// clang-format on

json config = defaultConfig;

json fixObject(json oldconfig, json newconfig) {
    for (auto oldField = oldconfig.begin(); oldField != oldconfig.end(); ++oldField) {
        auto newField = newconfig.find(oldField.key());

        // Add nonexisting fields
        if (newField == newconfig.end()) {
            printf("Config: No field %s \n", oldField.key().c_str());
            newconfig.emplace(oldField.key(), oldField.value());
            continue;
        }

        // Repair these with invalid types
        if (newField.value().type() != oldField.value().type()) {
            printf("[CONFIG]: Invalid type of field \"%s\" (%s), changing to %s\n", newField.key().c_str(), newField.value().type_name(),
                   oldField.value().type_name());
            newconfig[oldField.key()] = oldField.value();
            continue;
        }

        // If field is an object or an array - fix it recursively
        if (newField.value().is_object() || newField.value().is_array()) {
            newconfig[oldField.key()] = fixObject(oldField.value(), newField.value());
        }
    }
    return newconfig;
}

void saveConfigFile(const char* configName) { putFileContents(configName, config.dump(4)); }

void loadConfigFile(const char* configName) {
    auto file = getFileContents(configName);
    if (file.empty()) {
        saveConfigFile(configName);
        return;
    }
    nlohmann::json newconfig = json::parse(file.begin(), file.end());

    // Add missing and repair invalid fields
    config = fixObject(config, newconfig);
}

bool isEmulatorConfigured() { return !config["bios"].get<std::string>().empty(); }
