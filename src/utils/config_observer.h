#pragma once
#include <string>
#include <unordered_map>
#include <functional>

enum class Event {
    Controller,
};

class ConfigObserver {
    using Callback = std::function<void()>;
    std::unordered_map<Event, Callback> observers;

    public:
    void registerCallback(const Event name, Callback callback);
    void unregisterCallback(const Event name);
    void notify(const Event name);
};