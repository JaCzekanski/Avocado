#include "cache_control.h"
#include "system.h"

CacheControl::CacheControl(System* sys) : sys(sys) { reset(); }

void CacheControl::reset() { cache._reg = 0; }

uint32_t CacheControl::read(uint32_t address) { return cache._reg; }

void CacheControl::write(uint32_t address, uint32_t data) {
    cache._reg = data;
    sys->cpu->icacheEnabled = cache.icacheEnable;
}
