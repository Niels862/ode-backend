#include "analog-chip.hpp"
#include "analog-module.hpp"
#include "shadow-sram.hpp"
#include "io-port.hpp"
#include "settings.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include <iostream>
#include <string>
#include <fstream>
#include <iomanip>
#include <argp.h>

static argp_option options[] = {
    { "verbose",    'v', 0, 0,  "Use verbose output", 0 },
    { "raw",        'r', 0, 0,  "Write output in raw format", 0 },
    { "add-size",   's', 0, 0,  "Add size of configuration to output", 0 },
    { "add-check",  'c', 0, 0,  "Add check value at end of configuration", 0 },
    {}
};

static error_t parse_opt(int key, char *arg, argp_state *state) {
    switch (key) {
        case 'v':
            args.verbose = true;
            break;

        case 'r':
            args.raw = true;
            break;

        case 's':
            args.add_size = true;
            break;

        case 'c':
            args.add_check = true;
            break;

        case ARGP_KEY_ARG:
            switch (state->arg_num) {
                case 0: args.infile = arg; break;
                case 1: args.outfile = arg; break;
                default: argp_usage(state);
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

    if (args.raw) {
        if (args.verbose) {
            std::cerr << "Writing raw data..." << std::endl;
        }
        for (std::size_t bank_addr = 0; bank_addr <= 0x0A; bank_addr++) {
            for (std::size_t byte_addr = 0; byte_addr < 0x20; byte_addr++) {
                uint8_t byte = ssram.get(bank_addr, byte_addr).value();
                f << static_cast<int>(byte) << std::endl;
            }
        }
    } else {
        if (args.verbose) {
            std::cerr << "Writing C configuration..." << std::endl;
        }

        std::vector<uint8_t> data;
        chip.to_header_bytestream(data);
        ssram.to_data_bytestream(data);
    
        if (args.verbose) {
            std::cerr << "Bytestream length: " << data.size() << std::endl;
        }
#if 0    
        f << std::hex << std::setfill('0') << std::uppercase;
    
        f << "const unsigned char an_FPAA1_PrimaryConfigInfo[] = {\n";
        for (uint8_t byte : data) {
            f << "  0x" << static_cast<int>(byte) << ",\n";
        }
        f << "};\n";
#else
        if (args.add_size) {
            f << data.size() << std::endl;
        }
        for (uint8_t byte : data) {
            f << static_cast<int>(byte) << std::endl;
        }
        if (args.add_check) {
            f << 1337 << std::endl;
        }
#endif
    }

    f.close();
}

void load_doubling_sum(AnalogChip &chip) {
    chip.io_cell(1).set_mode(IOMode::InputBypass);

    chip.cab(3).setup(chip.clock(1), chip.null_clock());

    auto &suminv = chip.cab(3).add(new SumInv(0.32, 0.58));

    chip.io_cell(1).out(1).connect(suminv.in(1));
    chip.io_cell(1).out(1).connect(suminv.in(2));
}

void load_gain(AnalogChip &chip) {
    chip.io_cell(1).set_mode(IOMode::InputBypass);
    chip.io_cell(3).set_mode(IOMode::OutputBypass);

    chip.cab(1).setup(chip.clock(1), chip.null_clock());
    auto &gaininv = chip.cab(1).add(new GainInv(0.5));

    chip.io_cell(1).out(1).connect(gaininv.in(1));
    gaininv.opamp(1).out().connect(chip.io_cell(3).in(1));
}

void load_sum(AnalogChip &chip) {
    chip.io_cell(1).set_mode(IOMode::InputBypass);
    chip.io_cell(2).set_mode(IOMode::InputBypass);
    chip.io_cell(3).set_mode(IOMode::OutputBypass);

    chip.cab(1).setup(chip.clock(1), chip.null_clock());
    auto &suminv = chip.cab(1).add(new SumInv(1.0, 1.0));

    chip.io_cell(1).out(1).connect(suminv.in(1));
    chip.io_cell(2).out(1).connect(suminv.in(2));
    suminv.opamp(1).out().connect(chip.io_cell(3).in(1));
}

void load_gain_switch(AnalogChip &chip) {
    chip.io_cell(1).set_mode(IOMode::InputBypass);
    chip.io_cell(2).set_mode(IOMode::InputBypass);
    chip.io_cell(3).set_mode(IOMode::InputBypass);
    chip.io_cell(4).set_mode(IOMode::OutputBypass);

    chip.cab(1).setup(chip.clock(1), chip.null_clock());
    auto &gainswitch = chip.cab(1).add(new GainSwitch());

    chip.io_cell(1).out(1).connect(gainswitch.in(1));
    chip.io_cell(2).out(1).connect(gainswitch.in(2));
    chip.io_cell(3).out(1).connect(gainswitch.comp().in());
    gainswitch.opamp(1).out().connect(chip.io_cell(4).in(1));
}

void load_integgnd(AnalogChip &chip) {
    int io_inp = 1;
    int io_ctr = 2;
    int io_out = 3;
    int cab    = 1;

    chip.io_cell(io_inp).set_mode(IOMode::InputBypass);
    chip.io_cell(io_ctr).set_mode(IOMode::InputBypass);
    chip.io_cell(io_out).set_mode(IOMode::OutputBypass);

    chip.cab(cab).setup(chip.clock(1), chip.clock(3));
    auto &integ = chip.cab(cab).add(new Integrator(1.0, true));

    chip.io_cell(io_inp).out(1).connect(integ.in(1));
    chip.io_cell(io_ctr).out(1).connect(integ.comp().in());
    integ.opamp(1).out().connect(chip.io_cell(io_out).in(1));
}

void load_gain_switch_gnd(AnalogChip &chip) {
    int io_inp = 1;
    int io_ctr = 2;
    int io_out = 3;
    int cab    = 1;

    chip.io_cell(io_inp).set_mode(IOMode::InputBypass);
    chip.io_cell(io_ctr).set_mode(IOMode::InputBypass);
    chip.io_cell(io_out).set_mode(IOMode::OutputBypass);

    chip.cab(cab).setup(chip.clock(1), chip.null_clock());
    auto &integ = chip.cab(cab).add(new GainSwitch());

    chip.io_cell(io_inp).out(1).connect(integ.in(1));
    chip.io_cell(io_ctr).out(1).connect(integ.comp().in());
    integ.opamp(1).out().connect(chip.io_cell(io_out).in(1));
}

void load_sample_and_hold(AnalogChip &chip) {
    chip.io_cell(1).set_mode(IOMode::InputBypass);
    chip.io_cell(3).set_mode(IOMode::OutputBypass);

    chip.cab(1).setup(chip.clock(1), chip.null_clock());

    auto &hold = chip.cab(1).add(new SampleAndHold());
    
    chip.io_cell(1).out(1).connect(hold.in(1));
    hold.opamp(1).out().connect(chip.io_cell(3).in(1));
}

void load(AnalogChip &chip) {
    chip.io_cell(1).set_mode(IOMode::InputBypass);
    chip.io_cell(3).set_mode(IOMode::InputBypass);

    chip.cab(3).setup(chip.clock(1), chip.clock(3));

    auto &integ = chip.cab(3).add(new Integrator(0.42, true));

    chip.io_cell(1).out(1).connect(integ.in(1));
    chip.io_cell(3).out(1).connect(integ.comp().in());
}

std::unique_ptr<AnalogChip> parse_file(std::string filename) {
    Lexer lexer;
    std::vector<Token> tokens = lexer.lex(filename);

    Parser parser;
    std::unique_ptr<AnalogChip> chip = parser.parse(tokens);

    return chip;
}

int main(int argc, char *argv[]) {
    argp_parse(&argp, argc, argv, 0, 0, nullptr);

    auto chip = parse_file(args.infile);
    write(*chip);
    
    return 0;
}
