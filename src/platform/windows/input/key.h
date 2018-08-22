#pragma once
#include <SDL.h>
#include <string>

struct Key {
    enum class Type { None, Keyboard, MouseMove, MouseButton, ControllerMove, ControllerButton };
    enum class Axis { Invalid = 0, Up = 1, Right, Left, Down };

    Type type;
    union {
        SDL_Keycode key;

        struct {
            struct {
                Axis axis;
                int16_t value;
            };
            uint8_t button;
        } mouse;

        struct {
            int id;
            struct {
                int which;
                SDL_GameControllerAxis axis;
                int16_t value;
            };
            SDL_GameControllerButton button;
        } controller;
    };

    Key();
    Key(std::string& s);
    std::string to_string() const;
    static Key keyboard(SDL_Keycode keyCode);
    static Key mouseMove(SDL_MouseMotionEvent motion);
    static Key mouseButton(SDL_MouseButtonEvent button);
    static Key controllerMove(SDL_ControllerAxisEvent axis);
    static Key controllerButton(SDL_ControllerButtonEvent button);

   private:
    static std::string mapAxis(Axis axis);
    static std::string mapMouseButton(uint8_t button);
    static Axis stringToMouseAxis(std::string axis);
    static uint8_t stringToMouseButton(std::string button);
};