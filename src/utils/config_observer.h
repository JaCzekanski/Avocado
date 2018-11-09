#pragma once
#include <functional>
#include <string>
#include <unordered_map>

enum class Event {
    Controller,
    Graphics,
    Gpu,
};

class ConfigObserver {
    using Callback = std::function<void()>;
    // TODO: Allow multiple listeners for single callback
    std::unordered_map<Event, Callback> observers;

   public:
    void registerCallback(const Event name, Callback callback);
    void unregisterCallback(const Event name);
    void notify(const Event name);
};