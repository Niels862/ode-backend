#ifndef OBC_SHADOW_SRAM_HPP
#define OBC_SHADOW_SRAM_HPP

#include "memory-base.hpp"
#include "cab.hpp"
#include <iostream>

class ShadowSRam : public MemoryBase {
public:
    ShadowSRam();

    static std::size_t cab_bank_a(CAB::ID cab);
    static std::size_t cab_bank_b(CAB::ID cab);
};

#endif
