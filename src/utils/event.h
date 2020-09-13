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
    enum class Action {
        ask,
        slowboot,
        fastboot,
        swap,
    };
    std::string file;
    Action action = Action::ask;
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
namespace Debug {
struct OpenDrawListWindows {};
}  // namespace Debug
}  // namespace Gui

namespace Controller {
struct Vibration {
    int port;
    bool small;
    uint8_t big;
};

struct MemoryCardContentsChanged {
    int slot = 0;
};
struct MemoryCardSwapped {
    int slot = 0;
};
}  // namespace Controller

namespace Screenshot {
struct Save {
    std::string path;
    bool reset;
};
}  // namespace Screenshot
};  // namespace Event

void toast(const std::string& message);