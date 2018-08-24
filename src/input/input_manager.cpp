#include "input_manager.h"

InputManager* InputManager::_instance = nullptr;

bool InputManager::getDigital(const std::string& key) {
    if (auto s = state.find(key); s != state.end()) {
        return s->second.value > DIGITAL_THRESHOLD;
    }
    return false;
}

InputManager::AnalogValue InputManager::getAnalog(const std::string& key) {
    if (auto s = state.find(key); s != state.end()) {
        return s->second;
    }
    return {};
}

void InputManager::setInstance(InputManager* inputManager) { _instance = inputManager; }

InputManager* InputManager::getInstance() { return _instance; }