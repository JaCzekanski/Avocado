#pragma once
#include <string>
#include <unordered_map>
#include "device/controller/controller_type.h"
#include "device/gpu/rendering_mode.h"
#include "utils/event.h"

namespace avocado {
extern std::string PATH_DATA;  // Read-only directory, distributed with emulator
extern std::string PATH_USER;  // Writable directory for storing saves, config, etc

inline std::string assetsPath(const char* file = "") { return PATH_DATA + "assets/" + file; }
inline std::string shaderPath(const char* file = "") { return PATH_DATA + "shader/" + file; }
inline std::string biosPath() { return PATH_USER + "bios/"; }
inline std::string statePath(const char* file = "") { return PATH_USER + "state/" + file; }
inline std::string memoryPath(const char* file = "") { return PATH_USER + "memory/" + file; }
inline std::string isoPath(const char* file = "") { return PATH_USER + "iso/" + file; }
inline std::string screenshotPath(const char* file = "") { return PATH_USER + "screenshot/" + file; }
};  // namespace avocado

using KeyBindings = std::unordered_map<std::string, std::string>;

namespace DefaultKeyBindings {
KeyBindings none();
KeyBindings keyboard_wadx();
KeyBindings keyboard_numpad();
KeyBindings mouse();
KeyBindings controller(int n);
}  // namespace DefaultKeyBindings

namespace DefaultHotkeys {
KeyBindings keyboard();
}  // namespace DefaultHotkeys

struct avocado_config_t {
    std::string bios = "";
    std::string extension = "";
    std::string iso = "";

    KeyBindings hotkeys = DefaultHotkeys::keyboard();

    struct {
        ControllerType type;
        KeyBindings keys;
    } controller[2]{
        {
            ControllerType::analog,
            DefaultKeyBindings::keyboard_numpad(),
        },
        {
            ControllerType::none,
            DefaultKeyBindings::none(),
        },
    };

    struct {
        struct {
            RenderingMode renderingMode = RenderingMode::software;
            bool widescreen = false;
            bool forceWidescreen = false;
            struct {
                unsigned int width = 640;
                unsigned int height = 480;
            } resolution;
            bool vsync = false;
            bool forceNtsc = false;
            bool nativeTextureFormat = true;
        } graphics;

        struct {
            bool enabled = true;
        } sound;

        struct {
            bool preserveState = true;
            bool timeTravel = false;
        } emulator;

        struct {
            bool ram8mb = false;
        } system;

    } options;

    struct {
        struct {
            int bios = 0;
            int cdrom = 0;
            int controller = 0;
            int dma = 0;
            int gpu = 0;
            int gte = 0;
            int mdec = 0;
            int memoryCard = 0;
            int spu = 0;
            int system = 1;
            int memoryControl = 0;
        } log;
    } debug;

    struct {
        std::string path;
    } memoryCard[2]{
        {avocado::memoryPath("card1.mcr")},  //
        {""},                                //
    };

    struct {
        std::string lastPath = "";
    } gui;

    // Methods
    bool isEmulatorConfigured() const { return !bios.empty(); }
};

extern avocado_config_t config;