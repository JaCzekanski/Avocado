#pragma once
#include <functional>
#include <string>
#include <unordered_map>

enum class Event {
    Controller,
    Graphics,
};

class ConfigObserver {
    using Callback = std::function<void()>;
    std::unordered_map<Event, Callback> observers;

   public:
    void registerCallback(const Event name, Callback callback);
    void unregisterCallback(const Event name);
    void notify(const Event name);
};