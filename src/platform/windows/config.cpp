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
        {"debug", {
            {"log", {
                { "bios", 0 },
                { "cdrom", 0 }
            }}
        }}
};
// clang-format on

json config = defaultConfig;

void saveConfigFile(const char* configName) { putFileContents(configName, config.dump(4)); }

void loadConfigFile(const char* configName) {
    auto file = getFileContents(configName);
    if (file.empty()) {
        saveConfigFile(configName);
        return;
    }
    nlohmann::json newconfig = json::parse(file.begin(), file.end());

    for (auto it = config.begin(); it != config.end(); ++it) {
        auto field = newconfig.find(it.key());

        // Add nonexisting fields
        if (field == newconfig.end()) {
            printf("Config: No field %s \n", it.key().c_str());
            newconfig.emplace(it.key(), it.value());
            continue;
        }

        // Repair these with invalid types
        if (field.value().type() != it.value().type()) {
            printf("Config: Invalid type of %s (%s), changing to %s\n", field.key().c_str(), field.value().type_name().c_str(),
                   it.value().type_name().c_str());
            newconfig[it.key()] = it.value();
        }
    }

    config = newconfig;
}

bool isEmulatorConfigured() { return !config["bios"].get<std::string>().empty(); }
