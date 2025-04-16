#include "parser.hpp"
#include "error.hpp"
#include "defs.hpp"
#include "settings.hpp"

Parser::Parser(AnalogChip &chip)
        : m_chip{chip} {}

void Parser::parse(std::string const &infile) {
    m_file = std::ifstream(infile);
    if (!m_file) {
        throw DesignError("Could not open file");
    }

    m_modules.clear();

    parse_io_setup();
    parse_modules();
}

void Parser::parse_io_setup() {
    for (std::size_t i = 0; i < NType1IOCellsPerChip; i++) {
        std::string mode;
        m_file >> mode;

        if (mode == "input") {
            m_chip.io_cell(i + 1).set_mode(IOMode::InputBypass);
        } else if (mode == "output") {
            m_chip.io_cell(i + 1).set_mode(IOMode::OutputBypass);
        } else if (mode == "none") {
            m_chip.io_cell(i + 1).set_mode(IOMode::Disabled);
        }

        AnalogModule *module = &m_chip.io_cell(i + 1);
        m_modules["IO" + std::to_string(i + 1)] = module;
    }
}

void Parser::parse_modules() {
    std::string marker;
    m_file >> marker;
    if (marker != "start-modules") {
        throw DesignError("Expected start marker");
    }

    while (true) {
        std::string name;
        m_file >> name;

        if (name == "end-modules") {
            return;
        }
        if (args.verbose) {
            std::cerr << "Parsing `" << name << "`" << std::endl;
        }

        std::size_t cab;
        m_file >> cab;

        AnalogModule *module = AnalogModule::Build(name);
        if (!module) {
            throw DesignError("Unrecognized module name: " + name);
        }
        module->parse(m_file);
        m_chip.cab(cab).add(module);
    }
}

void Parser::parse_routing() {
    std::string marker;
    m_file >> marker;
    if (marker != "start-routing") {
        throw DesignError("Expected start marker");
    }

    while (true) {

    }
}