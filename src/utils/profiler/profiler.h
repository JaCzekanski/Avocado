#pragma once
#include <string>
#include <unordered_map>
#include <deque>

#define PROFILE(name)            \
    int ___PROFILE_##__LINE__##; \
    for (___PROFILE_##__LINE__## = 1, Profiler::start((name)); ___PROFILE_##__LINE__##; Profiler::stop((name)), --___PROFILE_##__LINE__##)

#define PROFILE_BLOCK(name) auto profiler##__LINE__## = ProfileFunction((name))
#define PROFILE_FUNCTION PROFILE_BLOCK(__FUNCTION__)

struct Sample {
    uint64_t start;
    uint64_t end;
};

struct Group {
    std::deque<Sample> samples;

    uint64_t total();
};

class Profiler {
    static bool enabled;
    static uint64_t startTime;

    static uint64_t getCounter();

   public:
    static std::unordered_map<const char*, Group> profiles;

    static void start(const char* name);
    static void stop(const char* name);
    static void reset();
    static void clear();

    static uint64_t getTotalTime();

    static void enable();
    static void disable();
    static bool isEnabled();
};

class ProfileFunction {
    const char* name;

   public:
    ProfileFunction(const char* name) : name(name) { Profiler::start(name); }

    ~ProfileFunction() { Profiler::stop(name); }
};