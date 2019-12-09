#pragma once
#include <SDL.h>
#include "debug/cdrom.h"
#include "debug/cpu.h"
#include "debug/gpu.h"
#include "debug/gte.h"
#include "debug/io.h"
#include "debug/kernel.h"
#include "debug/spu.h"
#include "debug/timers.h"
#include "file/file.h"
#include "help/about.h"
#include "help/sysinfo.h"
#include "options/memory_card.h"
#include "options/options.h"
#include "toasts.h"

struct System;

class GUI {
    SDL_Window* window;
    bool notInitializedWindowShown = false;

    gui::debug::Cdrom cdromDebug;
    gui::debug::CPU cpuDebug;
    gui::debug::GPU gpuDebug;
    gui::debug::Timers timersDebug;
    gui::debug::GTE gteDebug;
    gui::debug::SPU spuDebug;
    gui::debug::IO ioDebug;
    gui::help::About aboutHelp;
    gui::help::SysInfo sysinfoHelp;

    gui::options::MemoryCard memoryCardOptions;

    gui::Toasts toasts;

    void mainMenu(System* sys);

   public:
    bool singleFrame = false;
    bool showGui = true;

    GUI(SDL_Window* window, void* glContext);
    ~GUI();

    void processEvent(SDL_Event* e);
    void render(System* sys);
};