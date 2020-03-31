#include "config.h"
#include <fmt/core.h>
#include <nlohmann/json.hpp>
#include "device/controller/controller_type.h"
#include "device/gpu/rendering_mode.h"
#include "utils/file.h"
// TODO: json enums!

const char* CONFIG_NAME = "config.json";

namespace DefaultKeyBindings {
// clang-format off
avocado_config_t::KeyBindings none() {
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

avocado_config_t::KeyBindings keyboard_wadx() {
    return {
        {"dpad_up",    "keyboard|Up"},
        {"dpad_right", "keyboard|Right"},
        {"dpad_down",  "keyboard|Down"},
        {"dpad_left",  "keyboard|Left"},
        {"triangle",   "keyboard|w"},
        {"circle",     "keyboard|d"},
        {"cross",      "keyboard|x"},
        {"square",     "keyboard|a"},
        {"l1",         "keyboard|q"},
        {"r1",         "keyboard|e"},
        {"l2",         "keyboard|1"},
        {"r2",         "keyboard|3"},
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
avocado_config_t::KeyBindings keyboard_numpad() {
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

avocado_config_t::KeyBindings controller(int n) {
    auto C = [n](const char* key) {
        return fmt::format("controller{}|{}", n, key);
    };
    return {
        {"dpad_up",    C("dpup")},
        {"dpad_right", C("dpright")},
        {"dpad_down",  C("dpdown")},
        {"dpad_left",  C("dpleft")},
        {"triangle",   C("y")},
        {"circle",     C("b")},
        {"cross",      C("a")},
        {"square",     C("x")},
        {"l1",         C("leftshoulder")},
        {"r1",         C("rightshoulder")},
        {"l2",         C("+lefttrigger")},
        {"r2",         C("+righttrigger")},
        {"l3",         C("leftstick")},
        {"r3",         C("rightstick")},
        {"select",     C("back")},
        {"start",      C("start")},
        {"analog",     C("guide")},
        {"l_up",       C("-lefty")},
        {"l_right",    C("+leftx")},
        {"l_down",     C("+lefty")},
        {"l_left",     C("-leftx")},
        {"r_up",       C("-righty")},
        {"r_right",    C("+rightx")},
        {"r_down",     C("+righty")},
        {"r_left",     C("-rightx")},
    };
}

avocado_config_t::KeyBindings mouse() {
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

const avocado_config_t defaultConfig {
    .bios = "",
    .extension = "",
    .iso = "",
    .controller = {
        {
            .type  = ControllerType::analog,
            .keys = DefaultKeyBindings::keyboard_numpad(),
        },
        {
            .type  = ControllerType::none,
            .keys = DefaultKeyBindings::none(),
        },
    },
    .memoryCard = {
        {
            .path = "data/memory/card1.mcr"
        }, 
        {
            .path = ""
        },
    },
};

avocado_config_t config = defaultConfig;

using json = nlohmann::json;
json fixObject(json oldconfig, json newconfig) {
    for (auto oldField = oldconfig.begin(); oldField != oldconfig.end(); ++oldField) {
        auto newField = newconfig.find(oldField.key());

        // Add nonexisting fields
        if (newField == newconfig.end()) {
            newconfig.emplace(oldField.key(), oldField.value());
            continue;
        }

        // Repair these with invalid types
        if (newField.value().type() != oldField.value().type()) {
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

void saveConfigFile(const char* configName) {
    // putFileContents(configName, config.dump(4));
}

void loadConfigFile(const char* configName) {
    auto file = getFileContents(configName);
    if (file.empty()) {
        saveConfigFile(configName);
        return;
    }
    nlohmann::json newconfig = json::parse(file.begin(), file.end());

    // Add missing and repair invalid fields
    // config = fixObject(config, newconfig);
}

bool isEmulatorConfigured() { return !config.bios.empty(); }
