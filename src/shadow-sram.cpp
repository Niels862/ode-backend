#include "shadow-sram.hpp"

ShadowSRam::ShadowSRam()
        : MemoryBase{0x20U, 0x0bU, 0x00U} {}
