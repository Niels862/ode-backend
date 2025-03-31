#include "analog-chip.hpp"
#include "analog-module.hpp"
#include "shadow-sram.hpp"
#include "io-port.hpp"
#include <iostream>
#include <string>
#include <fstream>
#include <iomanip>

/* Will be expanded, for now just a function */
void write(AnalogChip const &chip, std::string const &filename) {
    std::ofstream f(filename);
    std::vector<uint8_t> data;

    ShadowSRam ssram = chip.compile();
    chip.to_header_bytestream(data);
    ssram.to_data_bytestream(data);

    f << std::hex << std::setfill('0') << std::uppercase;

    f << "const an_Byte an_FPAA1_PrimaryConfigInfo[] = \n";
    for (uint8_t byte : data) {
        f << "  0x" << static_cast<int>(byte) << ",\n";
    }
    f << "};\n";

    f.close();
}

int main() {
    AnalogChip chip;

    chip.io_cell(1).set_mode(IOMode::InputBypass);
    chip.io_cell(2).set_mode(IOMode::InputBypass);
    chip.io_cell(3).set_mode(IOMode::OutputBypass);

    auto &invsum = chip.cab(1).add(new InvSum(0.48, 4.18));
    auto &invgain = chip.cab(2).add(new InvGain(0.5));

    invsum.out().connect(invgain.in());
    invgain.out().connect(chip.io_cell(3).in());

    write(chip, "config.cpp");
    
    return 0;
}
