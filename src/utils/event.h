#pragma once
#include <eventbus/EventBus.h>
#include <string>

extern Dexode::EventBus bus;

namespace Event {
namespace Config {
struct Graphics {};
struct Gte {};
struct Controller {};
struct Spu {};
};  // namespace Config

namespace File {
struct Load {
    std::string file;
    bool reset;
};
struct Exit {};
};  // namespace File

namespace System {
struct SoftReset {};
struct HardReset {};
struct SaveState {
    int slot = 0;
};
struct LoadState {
    int slot = 0;
};
};  // namespace System

namespace Gui {
struct Toast {
    std::string message;
};
struct ToggleFullscreen {};
}  // namespace Gui

namespace Controller {
struct Vibration {
    int port;
    bool small;
    uint8_t big;
};
}  // namespace Controller
};  // namespace Event

void toast(const std::string& message);