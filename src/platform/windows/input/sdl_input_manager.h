#pragma once
#include <unordered_map>
#include "config.h"
#include "input/input_manager.h"
#include "key.h"

class SdlInputManager : public InputManager {
    std::unordered_map<int, SDL_GameController*> controllers;
    int32_t mouseX = 0;  // Track mouse movement in frame
    int32_t mouseY = 0;

    bool handleKey(Key key, AnalogValue value);
    void fixControllerId(SDL_Event& event);

   public:
    bool mouseCaptured = false;
    bool keyboardCaptured = false;
    bool mouseLocked = false;
    bool waitingForKeyPress = false;
    Key lastPressedKey;

    void newFrame();
    bool handleEvent(SDL_Event& event);
};