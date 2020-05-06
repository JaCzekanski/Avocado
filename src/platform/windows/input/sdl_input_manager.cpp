#include "sdl_input_manager.h"
#include <fmt/core.h>
#include "utils/math.h"

#if SDL_VERSION_ATLEAST(2, 0, 9)
#define VIBRATIONS_ENABLED
#else
#warning Vibrations are available for SDL >= 2.0.9
#endif

SdlInputManager::SdlInputManager() {
#ifdef VIBRATIONS_ENABLED
    vibrationThread = std::thread(&SdlInputManager::vibrationThreadFunc, this);
#endif
    busToken = bus.listen<Event::Controller::Vibration>(std::bind(&SdlInputManager::onVibrationEvent, this, std::placeholders::_1));
}

SdlInputManager::~SdlInputManager() {
    bus.unlistenAll(busToken);

#ifdef VIBRATIONS_ENABLED
    // Notify vibrationThread to stop execution
    {
        std::unique_lock<std::mutex> lk(vibrationMutex);
        vibrationThreadExit = true;
        vibrationHasNewData.notify_one();
    }
    vibrationThread.join();
#endif
}

void SdlInputManager::vibrationThreadFunc() {
    while (true) {
        std::unique_lock<std::mutex> lk(vibrationMutex);
        vibrationHasNewData.wait(lk);

        if (vibrationThreadExit) {
            break;
        }
#ifdef VIBRATIONS_ENABLED
        // TODO: Current solution might cut out vibration from different controllers
        // Buffer them and send all in bulk at the end of the frame ?
        SDL_GameControllerRumble(vibrationData.controller, vibrationData.e.big * 0xff, vibrationData.e.small * 0xffff, 16);
#endif
    }
}

void SdlInputManager::onVibrationEvent(Event::Controller::Vibration e) {
    // Vibration output is chosen by DPAD_UP mapped game controller
    std::string keyName = config.controller[e.port - 1].keys["dpad_up"];
    Key key(keyName);

    if (key.type != Key::Type::ControllerMove && key.type != Key::Type::ControllerButton) {
        return;  // DPAD_UP not mapped to a controller
    }

    auto controller = controllers[key.controller.id - 1];
    if (controller != nullptr) {
        std::unique_lock<std::mutex> lk(vibrationMutex);
        vibrationData = {controller, e};
        vibrationHasNewData.notify_one();
    }
}

bool SdlInputManager::isMouseBound() {
    // Check if mouse axis is bound anywhere
    for (auto& c : config.controller) {
        for (auto& k : c.keys) {
            if (k.second.rfind("mouse", 0) == 0) {
                return true;
            }
        }
    }
    return false;
}

bool SdlInputManager::handleKey(Key key, AnalogValue value) {
    if (waitingForKeyPress) {
        waitingForKeyPress = false;
        lastPressedKey = key;
        return true;
    }

    bool result = false;

    for (int c = 1; c <= 2; c++) {
        auto controller = config.controller[c - 1].keys;
        for (auto it = controller.begin(); it != controller.end(); ++it) {
            auto button = it->first;
            auto keyName = it->second;

            if (Key(keyName) == key) {
                auto path = fmt::format("controller/{}/{}", c, button);
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

#ifdef ANDROID
    shouldCaptureMouse = false;
#else
    shouldCaptureMouse = isMouseBound();
#endif

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
        && event.button.clicks == 2 && shouldCaptureMouse) {
        mouseLocked = true;
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
        fmt::print("Controller {} connected\n", SDL_GameControllerName(controllers[index]));
        return true;
    }

    if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
        int joystickId = event.cdevice.which;
        auto controller = SDL_GameControllerFromInstanceID(joystickId);
        for (auto it = controllers.begin(); it != controllers.end(); ++it) {
            if (it->second == controller) {
                fmt::print("Controller {} disconnected\n", SDL_GameControllerName(controller));
                SDL_GameControllerClose(controller);
                controllers.erase(it);
                return true;
            }
        }
    }

    return false;
}
