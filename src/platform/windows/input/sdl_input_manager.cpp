#include "sdl_input_manager.h"
#include "utils/math.h"
#include "utils/string.h"

bool SdlInputManager::handleKey(Key key, AnalogValue value) {
    if (waitingForKeyPress) {
        waitingForKeyPress = false;
        lastPressedKey = key;
        return true;
    }

    bool result = false;

    for (int c = 1; c <= 2; c++) {
        auto controller = config["controller"][std::to_string(c)]["keys"];
        for (auto it = controller.begin(); it != controller.end(); ++it) {
            auto button = it.key();
            auto keyName = it.value().get<std::string>();

            if (Key(keyName) == key) {
                auto path = string_format("controller/%d/%s", c, button.c_str());
                state[path.c_str()] = value;
                result = true;
            }
        }
    }

    return result;
}

void SdlInputManager::fixControllerId(SDL_Event& event) {
    int index = 0;
    auto controller = SDL_GameControllerFromInstanceID(event.cdevice.which);
    for (auto it = controllers.begin(); it != controllers.end(); ++it) {
        if (it->second == controller) {
            index = it->first;
            break;
        }
    }
    event.cbutton.which = index + 1;
}

void SdlInputManager::newFrame() {
    mouseX = 0;
    mouseY = 0;

    if (waitingForKeyPress) return;

    Key key;
    key.type = Key::Type::MouseMove;

    key.mouse.axis = Key::Axis::Right;
    handleKey(key, AnalogValue(0, true));

    key.mouse.axis = Key::Axis::Left;
    handleKey(key, AnalogValue(0, true));

    key.mouse.axis = Key::Axis::Up;
    handleKey(key, AnalogValue(0, true));

    key.mouse.axis = Key::Axis::Down;
    handleKey(key, AnalogValue(0, true));
}

bool SdlInputManager::handleEvent(SDL_Event& event) {
    auto type = event.type;

    if ((!mouseCaptured || waitingForKeyPress) && event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT
        && event.button.clicks == 2) {
#ifndef ANDROID
        mouseLocked = true;
#endif
        return true;
    }

    if ((!mouseCaptured || waitingForKeyPress) && (type == SDL_MOUSEBUTTONDOWN || type == SDL_MOUSEBUTTONUP)) {
        return handleKey(Key::mouseButton(event.button), AnalogValue(type == SDL_MOUSEBUTTONDOWN));
    }

    if ((!mouseCaptured || waitingForKeyPress) && type == SDL_MOUSEMOTION) {
        Key key;

        mouseX += event.motion.xrel;
        mouseY += event.motion.yrel;

        if (waitingForKeyPress) {
            if (std::abs(mouseX) > 10) {
                key = Key::mouseMove(clamp<int32_t>(mouseX, INT8_MIN, INT8_MAX), 0);
                return handleKey(key, AnalogValue(key.mouse.value));
            } else if (std::abs(mouseY) > 10) {
                key = Key::mouseMove(0, clamp<int32_t>(mouseY, INT8_MIN, INT8_MAX));
                return handleKey(key, AnalogValue(key.mouse.value));
            }
            return true;
        }

        // Mouse movement must be split into two separate events
        bool result = false;

        key = Key::mouseMove(clamp<int32_t>(mouseX, INT8_MIN, INT8_MAX), 0);
        result |= handleKey(key, AnalogValue(key.mouse.value, true));

        key = Key::mouseMove(0, clamp<int32_t>(mouseY, INT8_MIN, INT8_MAX));
        result |= handleKey(key, AnalogValue(key.mouse.value, true));

        return result;
    }

    if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
        switch (event.key.keysym.sym) {
            case SDLK_ESCAPE:
            case SDLK_LCTRL:
            case SDLK_LALT:
            case SDLK_RCTRL:
            case SDLK_RALT:
                if (mouseLocked) {
                    mouseLocked = false;
                    SDL_SetRelativeMouseMode(SDL_FALSE);
                    return true;
                }
                break;
        }
        if ((event.key.keysym.mod & KMOD_ALT) && event.key.keysym.sym == SDLK_RETURN) {
            bus.notify(Event::Gui::ToggleFullscreen{});
            return true;
        }
    }

    if ((!keyboardCaptured || waitingForKeyPress) && (type == SDL_KEYDOWN || type == SDL_KEYUP)) {
        return handleKey(Key::keyboard(event.key.keysym.sym), AnalogValue(type == SDL_KEYDOWN));
    }

    if (type == SDL_CONTROLLERAXISMOTION) {
        fixControllerId(event);
        if (waitingForKeyPress && !(std::abs(event.caxis.value) > 8192)) {
            return false;
        }
        auto key = Key::controllerMove(event.caxis);

        bool result = false;
        auto value = AnalogValue(static_cast<uint8_t>(key.controller.value >> 8));

        result |= handleKey(key, value);

        // Zero out other axis half
        key.controller.dir = !key.controller.dir;
        result |= handleKey(key, {});

        return result;
    }

    if (type == SDL_CONTROLLERBUTTONDOWN || type == SDL_CONTROLLERBUTTONUP) {
        fixControllerId(event);
        return handleKey(Key::controllerButton(event.cbutton), AnalogValue(type == SDL_CONTROLLERBUTTONDOWN));
    }

    if (event.type == SDL_CONTROLLERDEVICEADDED) {
        int index = event.cdevice.which;
        controllers[index] = SDL_GameControllerOpen(index);
        printf("Controller %s connected\n", SDL_GameControllerName(controllers[index]));
        return true;
    }

    if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
        int joystickId = event.cdevice.which;
        auto controller = SDL_GameControllerFromInstanceID(joystickId);
        for (auto it = controllers.begin(); it != controllers.end(); ++it) {
            if (it->second == controller) {
                printf("Controller %s disconnected\n", SDL_GameControllerName(controller));
                SDL_GameControllerClose(controller);
                controllers.erase(it);
                return true;
            }
        }
    }

    return false;
}