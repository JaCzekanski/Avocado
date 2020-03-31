#pragma once
#include <string>
#include <unordered_map>
#include "device/controller/controller_type.h"
#include "device/gpu/rendering_mode.h"
#include "utils/event.h"

#ifdef ANDROID
#include <android/log.h>
#define printf(...) __android_log_print(ANDROID_LOG_DEBUG, "AVOCADO", __VA_ARGS__);
#endif

using KeyBindings = std::unordered_map<std::string, std::string>;

namespace DefaultKeyBindings {
KeyBindings none();
KeyBindings keyboard_wadx();
KeyBindings keyboard_numpad();
KeyBindings mouse();
KeyBindings controller(int n);
}  // namespace DefaultKeyBindings

struct avocado_config_t {
    std::string bios = "";
    std::string extension = "";
    std::string iso = "";

    struct {
        ControllerType type;
        KeyBindings keys;
    } controller[2]{
        {
            .type = ControllerType::analog,
            .keys = DefaultKeyBindings::keyboard_numpad(),
        },
        {
            .type = ControllerType::none,
            .keys = DefaultKeyBindings::none(),
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
        } graphics;

        struct {
            bool enabled = true;
        } sound;

        struct {
            bool preserveState = true;
            bool timeTravel = false;
        } emulator;

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
        } log;
    } debug;

    struct {
        std::string path;
    } memoryCard[2]{
        {.path = "data/memory/card1.mcr"},  //
        {.path = ""},                       //
    };

    struct {
        std::string lastPath = "";
    } gui;

    // Methods
    bool isEmulatorConfigured() const { return !bios.empty(); }
};

extern avocado_config_t config;
