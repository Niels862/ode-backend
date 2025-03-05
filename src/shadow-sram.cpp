#include "shadow-sram.hpp"

ShadowSRam::ShadowSRam()
        : MemoryBase{0x20U, 0x0bU, 0x00U} {}

std::size_t ShadowSRam::cab_bank_a(CAB::ID cab) {
    switch (cab) {
        case CAB::CAB_1: return 3;
        case CAB::CAB_2: return 5;
        case CAB::CAB_3: return 7;
        case CAB::CAB_4: return 9;
    }

    return 0; /* error */
}

std::size_t ShadowSRam::cab_bank_b(CAB::ID cab) {
    switch (cab) {
        case CAB::CAB_1: return 4;
        case CAB::CAB_2: return 6;
        case CAB::CAB_3: return 8;
        case CAB::CAB_4: return 10;
    }

    return 0; /* error */
}
