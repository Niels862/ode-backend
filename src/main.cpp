#include "analog-chip.hpp"
#include "analog-module.hpp"
#include "shadow-sram.hpp"
#include "io-port.hpp"
#include "settings.hpp"
#include "parser.hpp"
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

    if (args.verbose) {
        std::cerr << "Bytestream length: " << data.size() << std::endl;
    }

    f << std::hex << std::setfill('0') << std::uppercase;

    f << "const unsigned char an_FPAA1_PrimaryConfigInfo[] = {\n";
    for (uint8_t byte : data) {
        f << "  0x" << static_cast<int>(byte) << ",\n";
    }
    f << "};\n";

    f.close();
}

void load(AnalogChip &chip) {
    chip.io_cell(3).set_mode(IOMode::OutputBypass);
    chip.io_cell(4).set_mode(IOMode::OutputBypass);

    chip.cab(1).setup(chip.clock(1), chip.clock(3));

    auto &gain1 = chip.cab(1).add(new GainInv(0.5));
    auto &gain2 = chip.cab(1).add(new GainInv(1.0));

    gain1.opamp(1).out().connect(chip.io_cell(3).in(1));
    gain2.opamp(1).out().connect(chip.io_cell(4).in(1));
}

void parse(AnalogChip &chip) {
    chip.cab(1).setup(chip.clock(1), chip.clock(3));
    Parser(chip).parse(args.infile);
}

int main(int argc, char *argv[]) {
    argp_parse(&argp, argc, argv, 0, 0, nullptr);

    AnalogChip chip;
    
    load(chip);
    write(chip);
    
    return 0;
}
