#include "analog-chip.hpp"
#include "analog-module.hpp"
#include "shadow-sram.hpp"
#include "io-port.hpp"
#include <iostream>

int main() {
    AnalogChip chip;
    ShadowSRam ssram;

    chip.io_cell(1).set_mode(IOMode::InputBypass);
    chip.io_cell(2).set_mode(IOMode::InputBypass);
    chip.io_cell(3).set_mode(IOMode::OutputBypass);

    InvertingSum invsum(chip.cab(1), 0.48, 4.18);
    
    chip.io_cell(1).out().connect(invsum.in_x());
    chip.io_cell(2).out().connect(invsum.in_y());
    invsum.out().connect(chip.io_cell(3).in());

    invsum.emit(ssram);

    std::cout << ssram << std::endl;

    return 0;
}
