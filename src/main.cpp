#include "shadow-sram.hpp"
#include "components.hpp"
#include <iostream>

int main() {
    ShadowSRam ssram;
    InvertingSum invsum(0.48, 4.18, CAB::CAB_1);

    invsum.emit(ssram);

    std::cout << ssram << std::endl;

    return 0;
}
