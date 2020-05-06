#pragma once
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <unordered_map>
#include "config.h"
#include "input/input_manager.h"
#include "key.h"

class SdlInputManager : public InputManager {
    int busToken;
    std::unordered_map<int, SDL_GameController*> controllers;
    int32_t mouseX = 0;  // Track mouse movement in frame
    int32_t mouseY = 0;
    bool shouldCaptureMouse = false;

    bool handleKey(Key key, AnalogValue value);
    void fixControllerId(SDL_Event& event);

    // Vibration stuff
    struct VibrationData {
        SDL_GameController* controller;
        Event::Controller::Vibration e;
    };

    std::atomic<bool> vibrationThreadExit = false;
    std::mutex vibrationMutex;
    std::condition_variable vibrationHasNewData;
    VibrationData vibrationData;
    std::thread vibrationThread;

    void vibrationThreadFunc();
    void onVibrationEvent(Event::Controller::Vibration e);

    bool isMouseBound();

   public:
    bool mouseCaptured = false;
    bool keyboardCaptured = false;
    bool mouseLocked = false;
    bool waitingForKeyPress = false;
    Key lastPressedKey;

    SdlInputManager();
    ~SdlInputManager();
    void newFrame();
    bool handleEvent(SDL_Event& event);
};