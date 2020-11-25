#include "key.h"
#include <fmt/core.h>
#include "utils/math.h"

std::string Key::mapAxis(Axis axis) {
    switch (axis) {
        case Axis::Invalid: return "";
        case Axis::Up: return "-y";
        case Axis::Right: return "+x";
        case Axis::Down: return "+y";
        case Axis::Left: return "-x";
    }
    return "";
}

std::string Key::mapMouseButton(uint8_t button) {
    switch (button) {
        case SDL_BUTTON_LEFT: return "Left";
        case SDL_BUTTON_RIGHT: return "Right";
        case SDL_BUTTON_MIDDLE: return "Middle";
        case SDL_BUTTON_X1: return "X1";
        case SDL_BUTTON_X2: return "X2";
    }
    return "";
}

Key::Axis Key::stringToMouseAxis(std::string axis) {
    if (axis == "-y")
        return Axis::Up;
    else if (axis == "+x")
        return Axis::Right;
    else if (axis == "+y")
        return Axis::Down;
    else if (axis == "-x")
        return Axis::Left;
    return Axis::Invalid;
}

uint8_t Key::stringToMouseButton(std::string button) {
    if (button == "Left")
        return SDL_BUTTON_LEFT;
    else if (button == "Right")
        return SDL_BUTTON_RIGHT;
    else if (button == "Middle")
        return SDL_BUTTON_MIDDLE;
    else if (button == "X1")
        return SDL_BUTTON_X1;
    else if (button == "X2")
        return SDL_BUTTON_X2;
    return 0;
}

std::string Key::to_string() const {
    std::string device;
    std::string event;

    if (type == Type::None) {
        return "";
    } else if (type == Type::Keyboard) {
        device = "keyboard";
        event = SDL_GetKeyName(key);
    } else if (type == Type::MouseMove) {
        device = "mouse";
        event = mapAxis(mouse.axis);
    } else if (type == Type::MouseButton) {
        device = "mouse";
        event = mapMouseButton(mouse.button);
    } else if (type == Type::ControllerMove) {
        device = fmt::format("controller{}", controller.id);
        event = (controller.dir ? '+' : '-');
        event += SDL_GameControllerGetStringForAxis(controller.axis);
    } else if (type == Type::ControllerButton) {
        device = fmt::format("controller{}", controller.id);
        event = SDL_GameControllerGetStringForButton(controller.button);
    }

    return device + '|' + event;
}

Key::Key() : type(Type::None) {}

Key::Key(std::string& s) {
    type = Type::None;

    auto delim = s.find('|');
    if (delim == std::string::npos) {
        return;
    }

    std::string device = s.substr(0, delim);
    std::string event = s.substr(delim + 1, s.length());

    if (device == "keyboard") {
        auto k = SDL_GetKeyFromName(event.c_str());
        if (k == SDLK_UNKNOWN) return;

        type = Type::Keyboard;
        key = k;
    } else if (device == "mouse") {
        auto a = stringToMouseAxis(event);
        if (a != Axis::Invalid) {
            type = Type::MouseMove;
            mouse.axis = a;
            return;
        }

        auto b = stringToMouseButton(event);
        if (b == 0) return;

        type = Type::MouseButton;
        mouse.button = b;
    } else if (device.rfind("controller", 0) == 0) {
        auto which = std::stoi(device.substr(std::string("controller").length()));

        if (event[0] == '+' || event[0] == '-') {
            controller.dir = event[0] == '+';
            event = event.substr(1);

            auto a = SDL_GameControllerGetAxisFromString(event.c_str());
            if (a != SDL_CONTROLLER_AXIS_INVALID) {
                type = Type::ControllerMove;
                controller.id = which;
                controller.axis = a;
                return;
            }
        }

        auto b = SDL_GameControllerGetButtonFromString(event.c_str());
        if (b != SDL_CONTROLLER_BUTTON_INVALID) {
            type = Type::ControllerButton;
            controller.id = which;
            controller.button = b;
            return;
        }

        return;
    }
}

Key Key::keyboard(SDL_Keycode keyCode) {
    Key key;
    key.type = Type::Keyboard;
    key.key = keyCode;

    return key;
}

Key Key::mouseMove(int8_t xrel, int8_t yrel) {
    Key key;
    key.type = Type::MouseMove;
    if (xrel > 0) {
        key.mouse.axis = Axis::Right;
        key.mouse.value = clamp<uint8_t>(xrel, 0, UINT8_MAX);
    } else if (xrel < 0) {
        key.mouse.axis = Axis::Left;
        key.mouse.value = clamp<uint8_t>(-xrel, 0, UINT8_MAX);
    } else if (yrel > 0) {
        key.mouse.axis = Axis::Down;
        key.mouse.value = clamp<uint8_t>(yrel, 0, UINT8_MAX);
    } else if (yrel < 0) {
        key.mouse.axis = Axis::Up;
        key.mouse.value = clamp<uint8_t>(-yrel, 0, UINT8_MAX);
    } else {
        key.type = Type::None;
    }

    return key;
}

Key Key::mouseButton(SDL_MouseButtonEvent button) {
    Key key;
    key.type = Type::MouseButton;
    key.mouse.button = button.button;

    return key;
}

Key Key::controllerMove(SDL_ControllerAxisEvent axis) {
    Key key;
    key.type = Type::ControllerMove;
    key.controller.id = axis.which;
    key.controller.dir = axis.value >= 0;
    key.controller.axis = static_cast<SDL_GameControllerAxis>(axis.axis);
    key.controller.value = std::abs(axis.value);

    return key;
}

Key Key::controllerButton(SDL_ControllerButtonEvent button) {
    Key key;
    key.type = Type::ControllerButton;
    key.controller.id = button.which;
    key.controller.button = static_cast<SDL_GameControllerButton>(button.button);

    return key;
}

bool Key::operator==(const Key& rhs) const {
    if (type != rhs.type) return false;

    if (type == Type::None) {
        return false;
    } else if (type == Type::Keyboard) {
        return key == rhs.key;
    } else if (type == Type::MouseMove) {
        return mouse.axis == rhs.mouse.axis;
    } else if (type == Type::MouseButton) {
        return mouse.button == rhs.mouse.button;
    } else if (type == Type::ControllerMove) {
        return (controller.id == rhs.controller.id) && (controller.axis == rhs.controller.axis) && (controller.dir == rhs.controller.dir);
    } else if (type == Type::ControllerButton) {
        return (controller.id == rhs.controller.id) && (controller.button == rhs.controller.button);
    }

    return false;
}

std::string Key::getButton() {
    auto button = to_string();
    size_t dpos = button.find('|');
    button = button.substr(dpos + 1);
    return button;
}

std::string Key::getDevice() {
    auto device = to_string();
    size_t dpos = device.find('|');
    device = device.substr(0, dpos);
    return device;
}