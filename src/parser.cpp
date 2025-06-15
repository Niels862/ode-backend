#include "parser.hpp"
#include <sstream>
#include <stdexcept>

Parser::Parser()
        : m_tokens{}, m_curr{}, m_opened{} {}

std::unique_ptr<AnalogChip> Parser::parse(std::vector<Token> tokens) {
    m_tokens = tokens;

    std::vector<std::unique_ptr<AnalogChip>> chips;

    while (!at_eof()) {
        std::cout << m_tokens[m_curr] << std::endl;

        if (matches(TokenType::Chip)) {
            auto chip = parse_chip();
            chips.push_back(std::move(chip));
        } else {
            expect_error("declaration");
        }
    }

    if (chips.size() < 1) {
        throw std::runtime_error("expected at least one chip");
    }
    return std::move(chips[0]);
}

void Parser::expect_error(std::string const &expected, std::string const &got) {
    std::stringstream ss;
    ss << "expected '" << expected << "', but got '" << got << "'";
    throw std::runtime_error(ss.str());
}

void Parser::expect_error(std::string const &expected) {
    expect_error(expected, to_string(m_tokens[m_curr].type()));
}

bool Parser::at_eof() const {
    return m_tokens[m_curr].type() == TokenType::EndOfFile;
}

Token &Parser::accept(TokenType type) {
    Token &token = m_tokens[m_curr];
    if (token.type() == type) {
        forward();
        return token;
    }

    static Token none(TokenType::None, "");
    return none;
}

Token &Parser::expect(TokenType type) {
    Token &token = m_tokens[m_curr];
    if (token.type() == type) {
        forward();
        return token;
    }

    expect_error(to_string(type));
}

bool Parser::matches(TokenType type) {
    return m_tokens[m_curr].type() == type;
}

void Parser::forward() {
    if (!at_eof()) {
        m_curr++;
    }
}

void Parser::open_attribute_map(std::string const &name) {
    expect(TokenType::LBrace);
    m_opened.emplace_back();
    m_opened_names.push_back(name);
}

bool Parser::has_next_attribute() {
    auto &attr_map = m_opened.back();
    if (!attr_map.empty()) {
        Token &comma = accept(TokenType::Comma);
        if (matches(TokenType::RBrace)) {
            return false;
        } else {
            if (!comma) {
                expect_error(to_string(TokenType::Comma));
            }
            return true;
        }
    }
    return !matches(TokenType::RBrace);
}

std::string Parser::parse_attribute() {
    auto &attr_map = m_opened.back();

    std::string attr = expect(TokenType::Identifier).lexeme();
    auto const &iter = attr_map.find(attr);
    if (iter != attr_map.end()) {
        std::stringstream ss;
        ss << "attribute '" << attr << "' was already declared";
        throw std::runtime_error(ss.str());
    }

    attr_map.insert(attr);

    expect(TokenType::Colon);
    
    return attr;
}

void Parser::unknown_attribute(std::string const &attr) {
    std::stringstream ss;
    ss << "attribute '" << attr << "' is not recognized for '" 
       << m_opened_names.back() << "'";
    throw std::runtime_error(ss.str());
}

void Parser::close_attribute_map() {
    expect(TokenType::RBrace);
    m_opened.pop_back();
    m_opened_names.pop_back();
}

std::unique_ptr<AnalogChip> Parser::parse_chip() {
    expect(TokenType::Chip);

    auto chip = std::make_unique<AnalogChip>();
    open_attribute_map("chip");

    while (has_next_attribute()) {
        std::string attr = parse_attribute();
        if (attr == "io") {
            parse_io_modes(*chip);
        } else {
            unknown_attribute(attr);
        }
    }

    close_attribute_map();

    return chip;
}

void Parser::parse_io_modes(AnalogChip &chip) {
    expect(TokenType::LSqBracket);

    std::size_t i = 1;
    while (true) {
        if (at_eof()) {
            expect(TokenType::RSqBracket);
        }

        if (i > NType1IOCellsPerChip) {
            std::stringstream ss;
            ss << "out of range: expected " << NType1IOCellsPerChip 
               << " specified modes";
            throw std::runtime_error(ss.str());
        }

        if (!accept(TokenType::Dash)) {
            std::string mode = expect(TokenType::Identifier).lexeme();
            if (mode == "input") {
                chip.io_cell(i).set_mode(IOMode::InputBypass);
            } else if (mode == "output") {
                chip.io_cell(i).set_mode(IOMode::OutputBypass);
            } else {
                std::stringstream ss;
                ss << "unknown mode: '" << mode << "'";
                throw std::runtime_error(ss.str());
            }
        }

        if (!accept(TokenType::Comma)) {
            expect(TokenType::RSqBracket);
            return;
        } else if (accept(TokenType::RSqBracket)) {
            return;
        }

        i++;
    }
}
