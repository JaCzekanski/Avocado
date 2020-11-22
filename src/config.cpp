#include "config.h"
#include <fmt/core.h>

namespace DefaultKeyBindings {
// clang-format off
KeyBindings none() {
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

KeyBindings keyboard_wadx() {
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
KeyBindings keyboard_numpad() {
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

KeyBindings controller(int n) {
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

KeyBindings mouse() {
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

namespace DefaultHotkeys {
KeyBindings keyboard() {
    return {
        {"toggle_menu", "keyboard|F1"},
        {"reset", "keyboard|F2"},
        {"close_tray", "keyboard|F3"},
        {"quick_save", "keyboard|F5"},
        {"single_frame", "keyboard|F6"},
        {"quick_load", "keyboard|F7"}, 
        {"single_step", "keyboard|F8"},
        {"toggle_pause", "keyboard|Space"},
        {"toggle_framelimit", "keyboard|Tab"},
        {"rewind_state", "keyboard|Backspace"},
        {"toggle_fullscreen", "keyboard|F4"}
    };
}
}  // namespace DefaultHotkeys

avocado_config_t config;

namespace avocado {
std::string PATH_DATA = "data/";
std::string PATH_USER = "data/";
}  // namespace avocado