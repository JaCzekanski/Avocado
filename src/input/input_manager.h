#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>

class InputManager {
   public:
    struct AnalogValue {
        uint8_t value;
        bool isRelative;

        AnalogValue(uint8_t value, bool isRelative = false) : value(value), isRelative(isRelative) {}
        AnalogValue(bool digital) : value(digital ? UINT8_MAX : 0), isRelative(false) {}
        AnalogValue() : value(0), isRelative(false) {}
    };

   private:
    const int DIGITAL_THRESHOLD = UINT8_MAX / 4;
    static InputManager* _instance;

   protected:
    std::unordered_map<std::string, AnalogValue> state;

   public:
    bool getDigital(const std::string& key);
    AnalogValue getAnalog(const std::string& key);

    static void setInstance(InputManager* inputManager);
    static InputManager* getInstance();
};