#pragma once
#include <eventbus/EventBus.h>
#include <string>

extern Dexode::EventBus bus;

namespace Event {
namespace Config {
struct Graphics {};
struct Gte {};
struct Controller {};
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
};  // namespace System

namespace Gui {
struct Toast {
    std::string message;
};
struct ToggleFullscreen {};
}  // namespace Gui
};  // namespace Event

void toast(const std::string& message);