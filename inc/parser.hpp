#ifndef OBC_PARSER_HPP
#define OBC_PARSER_HPP

#include "analog-chip.hpp"
#include <string>
#include <fstream>
#include <unordered_map>

class Parser {
public:
    Parser(AnalogChip &chip);

    void parse(std::string const &infile);

private:
    void parse_io_setup();
    void parse_modules();
    void parse_routing();

    InputPort *parse_input_port();
    OutputPort *parse_output_port();

    AnalogChip &m_chip;
    std::ifstream m_file;

    std::unordered_map<std::string, AnalogModule *> m_modules;
};

#endif
