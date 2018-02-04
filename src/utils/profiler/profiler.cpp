#include "profiler.h"
#include <SDL.h>

bool Profiler::enabled = false;
std::unordered_map<const char*, Group> Profiler::profiles;
uint64_t Profiler::startTime = 0;

uint64_t Group::total() {
    uint64_t sum = 0;
    for (auto& sample : samples) {
        sum += sample.end - sample.start;
    }
    return sum;
}

uint64_t Profiler::getCounter() { return SDL_GetPerformanceCounter(); }

void Profiler::start(const char* name) {
    if (!enabled) return;

    Sample s;
    s.end = 0;

    auto g = profiles.find(name);
    if (g == profiles.end()) {
        Group group;

        s.start = getCounter();
        group.samples.push_back(s);
        profiles[name] = group;
    } else {
        Group& group = g->second;

        s.start = getCounter();
        group.samples.push_back(s);
    }
}

void Profiler::stop(const char* name) {
    if (!enabled) return;
    uint64_t end = getCounter();

    auto g = profiles.find(name);
    if (g == profiles.end()) {
        throw std::exception("Calling Profiler::stop for non-existing group name");
    }

    Group& group = g->second;
    Sample& s = group.samples.back();
    s.end = end;
}

void Profiler::reset() {
    for (auto& profile : profiles) {
        profile.second.samples.clear();
    }
    startTime = getCounter();
}

void Profiler::clear() { profiles.clear(); }

uint64_t Profiler::getTotalTime() { return getCounter() - startTime; }

void Profiler::enable() { enabled = true; }

void Profiler::disable() { enabled = false; }

bool Profiler::isEnabled() { return enabled; }