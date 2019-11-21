#pragma once
#include "device.h"

struct System;

union CacheConfiguration {
    struct {
        uint32_t lock : 1;        // lock, Tag Test Mode
        uint32_t inv : 1;         // inv, Invalidate Mode
        uint32_t tag : 1;         // tag, Tag Test Mode (used by BIOS when invalidating icache)
        uint32_t scratchpad : 1;  // ram, use dcache as scratchpad (ignore valid bits)

        uint32_t dcacheRefillSize : 2;  // dblksz
        uint32_t : 1;
        uint32_t dcacheEnable : 1;  // ds, used with bit3 to enable scratchpad

        uint32_t icacheRefillSize : 2;  // iblksz
        uint32_t is0 : 1;               // is0, enable icache set 0 - cleared to 0
        uint32_t icacheEnable : 1;      // is1, enable icache set 1 - 1 to enable

        uint32_t interruptPolarity : 1;   // intp, not used, should be 0
        uint32_t enableReadPriority : 1;  // rdpri, loads operations will have priority over store
        uint32_t noWaitState : 1;         // nopad
        uint32_t enableBusGrant : 1;      // bgnt, should be 0

        uint32_t loadScheduling : 1;  // ldsch
        uint32_t noStreaming : 1;     // nostr, not used, should be 0

        uint32_t : 14;
    };
    uint32_t _reg;

    template <class Archive>
    void serialize(Archive& ar) {
        ar(_reg);
    }
};

class CacheControl {
    System* sys;

    CacheConfiguration cache;

   public:
    CacheControl(System* sys);
    void reset();
    uint32_t read(uint32_t address);
    void write(uint32_t address, uint32_t data);

    template <class Archive>
    void serialize(Archive& ar) {
        ar(cache);
    }
};