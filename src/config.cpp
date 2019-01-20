#include "config.h"
#include "utils/file.h"

const char* CONFIG_NAME = "config.json";

namespace ControllerType {
const std::string NONE = "none";
const std::string DIGITAL = "digital";
const std::string ANALOG = "analog";
const std::string MOUSE = "mouse";
}  // namespace ControllerType

namespace DefaultKeyBindings {
// clang-format off
json none() {
    return {
        {"dpad_up",    ""},
        {"dpad_right", ""},
        {"dpad_down",  ""},
        {"dpad_left",  ""},
        {"triangle",   ""},
        {"circle",     ""},
        {"cross",      ""},
        {"square",     ""},
        {"l1",         ""},
        {"r1",         ""},
        {"l2",         ""},
        {"r2",         ""},
        {"l3",         ""},
        {"r3",         ""},
        {"select",     ""},
        {"start",      ""},
        {"analog",     ""},
        {"l_up",       ""},
        {"l_right",    ""},
        {"l_down",     ""},
        {"l_left",     ""},
        {"r_up",       ""},
        {"r_right",    ""},
        {"r_down",     ""},
        {"r_left",     ""},
    };
}

json keyboard_numpad() {
    return {
        {"dpad_up",    "keyboard|Up"},
        {"dpad_right", "keyboard|Right"},
        {"dpad_down",  "keyboard|Down"},
        {"dpad_left",  "keyboard|Left"},
        {"triangle",   "keyboard|Keypad 8"},
        {"circle",     "keyboard|Keypad 6"},
        {"cross",      "keyboard|Keypad 2"},
        {"square",     "keyboard|Keypad 4"},
        {"l1",         "keyboard|Keypad 7"},
        {"r1",         "keyboard|Keypad 9"},
        {"l2",         "keyboard|Keypad /"},
        {"r2",         "keyboard|Keypad *"},
        {"l3",         ""},
        {"r3",         ""},
        {"select",     "keyboard|Right Shift"},
        {"start",      "keyboard|Return"},
        {"analog",     ""},
        {"l_up",       ""},
        {"l_right",    ""},
        {"l_down",     ""},
        {"l_left",     ""},
        {"r_up",       ""},
        {"r_right",    ""},
        {"r_down",     ""},
        {"r_left",     ""},
    };
}

json controller() {
    return {
        {"dpad_up",    "controller1|dpup"},
        {"dpad_right", "controller1|dpright"},
        {"dpad_down",  "controller1|dpdown"},
        {"dpad_left",  "controller1|dpleft"},
        {"triangle",   "controller1|y"},
        {"circle",     "controller1|b"},
        {"cross",      "controller1|a"},
        {"square",     "controller1|x"},
        {"l1",         "controller1|leftshoulder"},
        {"r1",         "controller1|rightshoulder"},
        {"l2",         "controller1|+lefttrigger"},
        {"r2",         "controller1|+righttrigger"},
        {"l3",         "controller1|leftstick"},
        {"r3",         "controller1|rightstick"},
        {"select",     "controller1|back"},
        {"start",      "controller1|start"},
        {"analog",     "controller1|guide"},
        {"l_up",       "controller1|-lefty"},
        {"l_right",    "controller1|+leftx"},
        {"l_down",     "controller1|+lefty"},
        {"l_left",     "controller1|-leftx"},
        {"r_up",       "controller1|-righty"},
        {"r_right",    "controller1|+rightx"},
        {"r_down",     "controller1|+righty"},
        {"r_left",     "controller1|-rightx"},
    };
}

json mouse() {
    return {
        {"dpad_up",    ""},
        {"dpad_right", ""},
        {"dpad_down",  ""},
        {"dpad_left",  ""},
        {"triangle",   ""},
        {"circle",     ""},
        {"cross",      ""},
        {"square",     ""},
        {"l1",         "mouse|Left"},
        {"r1",         "mouse|Right"},
        {"l2",         ""},
        {"r2",         ""},
        {"l3",         ""},
        {"r3",         ""},
        {"select",     ""},
        {"start",      ""},
        {"analog",     ""},
        {"l_up",       "mouse|-y"},
        {"l_right",    "mouse|+x"},
        {"l_down",     "mouse|+y"},
        {"l_left",     "mouse|-x"},
        {"r_up",       ""},
        {"r_right",    ""},
        {"r_down",     ""},
        {"r_left",     ""},
    };
}
// clang-format on
}  // namespace DefaultKeyBindings

// clang-format off
const json defaultConfig = {
	{"bios", ""},
	{"extension", ""},
	{"iso", ""},
    {"controller", {
        {"1", {
            {"type", ControllerType::ANALOG},
            {"keys", DefaultKeyBindings::keyboard_numpad()}
        }},
        {"2", {
            {"type", ControllerType::NONE},
            {"keys", DefaultKeyBindings::none()}
        }}
    }},
    {"options", {
        {"graphics", {
            {"rendering_mode", RenderingMode::SOFTWARE},
            {"filtering", false},
            {"widescreen", false},
            {"forceWidescreen", false},
            {"resolution", {
                {"width", 640u},
                {"height", 480u}
            }},
            {"vsync", false}
        }}
    }},
    {"debug", {
        {"log", {
            { "system",1u },
            { "bios",  0u },
            { "cdrom", 0u },
            { "memoryCard", 1u },
            { "controller", 1u },
            { "dma", 0u }
        }}
    }},
    {"memoryCard", {
        {"1", "data/memory/card1.mcr"},
        {"2", nullptr}
    }}
};
// clang-format on

ConfigObserver configObserver;

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
