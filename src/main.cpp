#include "analog-chip.hpp"
#include "analog-module.hpp"
#include "shadow-sram.hpp"
#include "io-port.hpp"
#include "settings.hpp"
#include <iostream>
#include <string>
#include <fstream>
#include <iomanip>
#include <argp.h>

static argp_option options[] = {
    { "verbose",    'v', 0, 0,  "Use verbose output", 0 },
    {}
};

static error_t parse_opt(int key, char *arg, argp_state *state) {
    switch (key) {
        case 'v':
            args.verbose = true;
            break;

        case ARGP_KEY_ARG:
            if (args.infile.empty()) {
                args.infile = arg;
            } else if (args.outfile.empty()) {
                args.outfile = arg;
            } else {
                argp_usage(state);
            }
            break;

        case ARGP_KEY_END:
            if (args.outfile.empty()) {
                argp_usage(state);
            }
            // end conditions
            break;

        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = { 
    options, parse_opt, nullptr, nullptr, nullptr, nullptr, nullptr 
};

/* Will be expanded, for now just a function */
void write(AnalogChip &chip) {
    std::ofstream f(args.outfile);

    ShadowSRam ssram = chip.compile();

    if (args.verbose) {
        std::cerr << ssram << std::endl;
    }

    std::vector<uint8_t> data;
    chip.to_header_bytestream(data);
    ssram.to_data_bytestream(data);

    f << std::hex << std::setfill('0') << std::uppercase;

    f << "const unsigned char an_FPAA1_PrimaryConfigInfo[] = {\n";
    for (uint8_t byte : data) {
        f << "  0x" << static_cast<int>(byte) << ",\n";
    }
    f << "};\n";

    f.close();
}

int main(int argc, char *argv[]) {
    argp_parse(&argp, argc, argv, 0, 0, nullptr);

    AnalogChip chip;

    chip.io_cell(1).set_mode(IOMode::InputBypass);
    chip.io_cell(2).set_mode(IOMode::InputBypass);
    chip.io_cell(3).set_mode(IOMode::OutputBypass);

    auto &invsum = chip.cab(1).add(new InvSum(0.48, 3.14159));
    auto &invgain = chip.cab(2).add(new InvGain(0.5));

    chip.io_cell(1).out().connect(invsum.in(1));
    chip.io_cell(2).out().connect(invsum.in(2));

    invsum.out().connect(invgain.in());
    invgain.out().connect(chip.io_cell(3).in());

    /*
    chip.io_cell(1).set_mode(IOMode::InputBypass);
    chip.io_cell(3).set_mode(IOMode::OutputBypass);

    auto &invgain1 = chip.cab(1).add(new InvGain(1));
    auto &invgain2 = chip.cab(3).add(new InvGain(1));

    chip.io_cell(1).out().connect(invgain1.in());
    invgain1.out().connect(invgain2.in());
    invgain2.out().connect(chip.io_cell(3).in());
    */

    write(chip);
    
    return 0;
}
