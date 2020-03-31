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

struct avocado_config_t {
    using string = std::string;
    using KeyBindings = std::unordered_map<std::string, std::string>;

    string bios = "";
    string extension = "";
    string iso = "";

    struct {
        ControllerType type;
        KeyBindings keys;
    } controller[2];

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
            bool timeTravel = false;  // Change it?
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
        string path;
    } memoryCard[2];

    struct {
        string lastPath = "";
    } gui;
};

extern avocado_config_t config;

namespace DefaultKeyBindings {
avocado_config_t::KeyBindings none();
avocado_config_t::KeyBindings keyboard_wadx();
avocado_config_t::KeyBindings keyboard_numpad();
avocado_config_t::KeyBindings mouse();
avocado_config_t::KeyBindings controller(int n);
}  // namespace DefaultKeyBindings

extern const char* CONFIG_NAME;
extern const avocado_config_t defaultConfig;
// extern nlohmann::json config;
void saveConfigFile(const char* configName);
void loadConfigFile(const char* configName);

bool isEmulatorConfigured();