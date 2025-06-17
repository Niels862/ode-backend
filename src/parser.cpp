#include "parser.hpp"
#include <sstream>
#include <stdexcept>
#include <cmath>

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

void Parser::expect_error(std::string const &expected) {
    Token &token = m_tokens[m_curr];
    std::stringstream ss;
    ss << "expected " << expected << ", but got " << token.type();
    token.error(ss.str());
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

    static Token none;
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

Token &Parser::parse_attribute() {
    auto &attr_map = m_opened.back();

    Token &attr = expect(TokenType::Identifier);
    auto const &iter = attr_map.find(attr.lexeme());
    if (iter != attr_map.end()) {
        std::stringstream ss;
        ss << "attribute '" << attr << "' was already declared";
        throw std::runtime_error(ss.str());
    }

    attr_map.insert(attr.lexeme());

    expect(TokenType::Colon);
    
    return attr;
}

void Parser::unknown_attribute(Token const &attr) {
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


bool Parser::open_list() {
    expect(TokenType::LSqBracket);
    return !accept(TokenType::RSqBracket);
}

bool Parser::is_list_end() {
    if (!accept(TokenType::Comma)) {
        expect(TokenType::RSqBracket);
        return true;
    }
     
    if (accept(TokenType::RSqBracket)) {
        return true;
    } 
    
    if (at_eof()) {
        expect(TokenType::RSqBracket);
    }

    return false;
}

std::unique_ptr<AnalogChip> Parser::parse_chip() {
    expect(TokenType::Chip);

    auto chip = std::make_unique<AnalogChip>();
    open_attribute_map("chip");

    while (has_next_attribute()) {
        Token &attr = parse_attribute();
        if (attr == "io") {
            parse_io_modes(*chip);
        } else if (attr == "cabs") {
            parse_cabs_list(*chip);
        } else {
            unknown_attribute(attr);
        }
    }

    close_attribute_map();

    return chip;
}

void Parser::parse_io_modes(AnalogChip &chip) {
    if (!open_list()) {
        return;
    }

    std::size_t i = 1;
    while (true) {
        if (i > NType1IOCellsPerChip) {
            std::stringstream ss;
            ss << "out of range: expected " << NType1IOCellsPerChip 
               << " specified modes";
            throw std::runtime_error(ss.str());
        }

        if (!accept(TokenType::Dash)) {
            Token &mode = expect(TokenType::Identifier);
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

        if (is_list_end()) {
            return;
        }

        i++;
    }
}

void Parser::parse_cabs_list(AnalogChip &chip) {
    if (!open_list()) {
        return;
    }

    while (true) {
        expect(TokenType::Cab);

        int32_t id = 
                parse_ranged_integer_expression(1, NBlocksPerChip, "cab id");

        AnalogBlock &cab = chip.cab(id);
        parse_cab_setup(chip, cab);
        parse_cab(cab);
        
        if (is_list_end()) {
            return;
        }
    }
}

void Parser::parse_cab_setup(AnalogChip &chip, AnalogBlock &cab) {
    expect(TokenType::With);
    expect(TokenType::Clocks);

    Clock &clk_a = parse_clock_id(chip);
    expect(TokenType::Comma);
    Clock &clk_b = parse_clock_id(chip);

    cab.setup(clk_a, clk_b);
}

Clock &Parser::parse_clock_id(AnalogChip &chip) {
    if (accept(TokenType::Dash)) {
        return chip.null_clock();
    }

    std::size_t id = parse_ranged_integer_expression(1, 6, "clock id");
    return chip.clock(id);
}

void Parser::parse_cab(AnalogBlock &cab) {
    open_attribute_map("cab");

    while (has_next_attribute()) {
        Token &attr = parse_attribute();
        if (attr == "cams") {
            parse_cam_list(cab);
        } else {
            unknown_attribute(attr);
        }
    }

    close_attribute_map();
}

void Parser::parse_cam_list(AnalogBlock &) {
    if (!open_list()) {
        return;
    }

    while (true) {
        expect(TokenType::Dash);

        if (is_list_end()) {
            return;
        }
    }
}

double Parser::parse_expression() {
    Token &num = expect(TokenType::Number);

    try {
        return std::stod(std::string(num.lexeme())); // todo improve stod
    } catch (...) {
        throw std::runtime_error("malformed number");
    }
}

int64_t Parser::parse_ranged_integer_expression(int64_t lo, int64_t hi, 
                                                std::string ctx) {
    int64_t value = parse_integer_expression();

    if (value < lo || value > hi) {
        std::stringstream ss;
        ss << value << " is out of range for " << ctx << ": " 
           << lo << " <= value <= " << hi << std::endl; 
        throw std::runtime_error(ss.str());
    }

    return value;
}

int64_t Parser::parse_integer_expression() {
    return std::llround(parse_expression());
}

double Parser::parse_double_expression() {
    return parse_expression();
}
