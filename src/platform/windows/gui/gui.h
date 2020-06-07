#pragma once
#include <SDL.h>
#include <optional>
#include "file/open.h"
#include "debug/cdrom.h"
#include "debug/cpu.h"
#include "debug/gpu.h"
#include "debug/gte.h"
#include "debug/io.h"
#include "debug/kernel.h"
#include "debug/spu.h"
#include "debug/timers.h"
#include "help/about.h"
#include "options/bios.h"
#include "options/memory_card.h"
#include "options/options.h"
#include "toasts.h"

struct System;

class GUI {
    SDL_Window* window;
    bool notInitializedWindowShown = false;
    std::string iniPath;

    gui::file::Open openFile;

    gui::debug::Cdrom cdromDebug;
    gui::debug::CPU cpuDebug;
    gui::debug::GPU gpuDebug;
    gui::debug::Timers timersDebug;
    gui::debug::GTE gteDebug;
    gui::debug::SPU spuDebug;
    gui::debug::IO ioDebug;

    gui::options::Bios biosOptions;
    gui::options::MemoryCard memoryCardOptions;

    gui::help::About aboutHelp;

    gui::Toasts toasts;

    void mainMenu(std::unique_ptr<System>& sys);
    void discDialog();
    void drawControls(std::unique_ptr<System>& sys);
    void renderController();

   public:
    static float scale;

    bool singleFrame = false;
    bool showGui = true;
    bool showMenu = true;

    // Status
    double statusFps = 0.0;
    bool statusFramelimitter = true;
    bool statusMouseLocked = false;

    // Drag&drop
    std::optional<std::string> droppedItem;
    bool droppedItemDialogShown = false;

    GUI(SDL_Window* window, void* glContext);
    ~GUI();

    void processEvent(SDL_Event* e);
    void render(std::unique_ptr<System>& sys);
};