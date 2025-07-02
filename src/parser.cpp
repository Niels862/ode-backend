#include "parser.hpp"
#include <sstream>
#include <stdexcept>
#include <cmath>

Parser::Parser()
        : m_tokens{}, m_curr{}, m_opened{},
          m_chip_cams{}, m_named_consts{} {}

std::unique_ptr<AnalogChip> Parser::parse(std::vector<Token> tokens) {
    m_tokens = tokens;

    std::vector<std::unique_ptr<AnalogChip>> chips;

    while (!at_eof()) {
        if (matches(TokenType::Let)) {
            parse_let_declaration();
        } else if (matches(TokenType::Chip)) {
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

void Parser::check_shadowed_definition(Token &name) {
    if (m_chip_cams.find(name.lexeme()) != m_chip_cams.end()
        || m_named_consts.find(name.lexeme()) != m_named_consts.end()) {
        std::stringstream ss;
        ss << "declaration of '" << name.lexeme() 
           << "' shadows previous definition";
        name.error(ss.str());
    }
}

AnalogModule *Parser::find_cam(AnalogChip &chip, Token &name) {
    static const std::unordered_map<std::string_view, int> io_map {
        { "io1", 1 },
        { "io2", 2 },
        { "io3", 3 },
        { "io4", 4 },
    };

    std::string_view const &lexeme = name.lexeme();

    auto io_iter = io_map.find(lexeme);
    if (io_iter != io_map.end()) {
        return &chip.io_cell(io_iter->second);
    }

    auto cam_iter = m_chip_cams.find(lexeme);
    if (cam_iter != m_chip_cams.end()) {
        return cam_iter->second;
    }

    std::stringstream ss;
    ss << lexeme << " is not a CAM";
    name.error(ss.str());
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

void Parser::unknown_attribute(Token &attr) {
    std::stringstream ss;
    ss << "attribute '" << attr.lexeme() << "' is not recognized for '" 
       << m_opened_names.back() << "'";
    attr.error(ss.str());
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

void Parser::parse_let_declaration() {
    expect(TokenType::Let);
    
    Token &name = expect(TokenType::Identifier);
    check_shadowed_definition(name);

    expect(TokenType::Equals);
    double value = parse_double_expression();
    expect(TokenType::Semicolon);

    m_named_consts[name.lexeme()] = value;
}

std::unique_ptr<AnalogChip> Parser::parse_chip() {
    expect(TokenType::Chip);

    auto chip = std::make_unique<AnalogChip>();
    m_chip_cams = {};

    open_attribute_map("chip");

    while (has_next_attribute()) {
        Token &attr = parse_attribute();
        if (attr == "io") {
            parse_io_modes(*chip);
        } else if (attr == "cabs") {
            parse_cabs_list(*chip);
        } else if (attr == "routing") {
            parse_routing(*chip);
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

        if (!accept(TokenType::Minus)) {
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
    if (accept(TokenType::Minus)) {
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

void Parser::parse_cam_list(AnalogBlock &cab) {
    if (!open_list()) {
        return;
    }

    while (true) {
        parse_cam(cab);
        
        if (is_list_end()) {
            return;
        }
    }
}

void Parser::parse_cam(AnalogBlock &cab) {
    Token &name = expect(TokenType::Identifier);
    expect(TokenType::As);
    Token &key = expect(TokenType::Identifier);

    AnalogModule *cam = AnalogModule::Build(name.lexeme());

    if (!cam) {
        std::stringstream ss;
        ss << "undefined CAM name: " << name.lexeme();
        name.error(ss.str());
    }
    cab.add_raw(cam);

    if (m_chip_cams.find(key.lexeme()) != m_chip_cams.end()) {
        std::stringstream ss;
        ss << "shadowed CAM key: " << name.lexeme();
        key.error(ss.str());
    }

    m_chip_cams[key.lexeme()] = cam;

    open_attribute_map(std::string(name.lexeme()));

    while (has_next_attribute()) {
        Token &attr = parse_attribute();
        double value = parse_double_expression();
        if (!cam->set_parameter(attr.lexeme(), Parameter(value))) {
            unknown_attribute(attr);
        }
    }

    close_attribute_map();

    cam->claim_components();
}

void Parser::parse_routing(AnalogChip &chip) {
    if (!open_list()) {
        return;
    }

    while (true) {
        parse_routing_entry(chip);

        if (is_list_end()) {
            return;
        }
    }
}

void Parser::parse_routing_entry(AnalogChip &chip) {
    OutputPort &out = parse_output_port(chip);
    Token &arrow = expect(TokenType::Arrow);
    InputPort &in = parse_input_port(chip);
    try {
        out.connect(in);
    } catch (std::exception const &e) {
        arrow.error(e.what());
    }
}

OutputPort &Parser::parse_output_port(AnalogChip &chip) {
    Token &name = expect(TokenType::Identifier);
    AnalogModule *cam = find_cam(chip, name);
    if (accept(TokenType::Colon)) {
        int64_t port = parse_integer_expression();
        return cam->out(port); // fixme 
    }
    return cam->out();
}

InputPort &Parser::parse_input_port(AnalogChip &chip) {
    Token &name = expect(TokenType::Identifier);
    AnalogModule *cam = find_cam(chip, name);
    if (accept(TokenType::Colon)) {
        if (accept(TokenType::Cmp)) {
            return cam->comp().in();
        } else {
            int64_t port = parse_integer_expression();
            return cam->in(port); // fixme 
        }
    }
    return cam->in();
}

double Parser::parse_expression() {
    return parse_sum();
}

double Parser::parse_sum() {
    double value = parse_term();
    while (true) {
        if (accept(TokenType::Plus)) {
            value = value + parse_term();
        } else if (accept(TokenType::Minus)) {
            value = value - parse_term();
        } else {
            return value;
        }
    }
}

double Parser::parse_term() {
    double value = parse_unary();
    while (true) {
        if (accept(TokenType::Asterisk)) {
            value = value * parse_unary();
        } else if (accept(TokenType::Slash)) {
            value = value / parse_unary();
        } else {
            return value;
        }
    }
}

double Parser::parse_unary() {
    if (accept(TokenType::Minus)) {
        return -parse_unary();
    }
    return parse_atom();
}

double Parser::parse_atom() {
    if (Token &num = accept(TokenType::Number)) {
        try {
            return std::stod(std::string(num.lexeme())); // todo improve stod
        } catch (...) {
            std::stringstream ss;
            ss << "malformed number: " << num.lexeme();
            num.error(ss.str());
        }
    } 
    if (accept(TokenType::True)) {
        return 1.0;
    } 
    if (accept(TokenType::False)) {
        return 0.0;
    } 
    if (Token &name = accept(TokenType::Identifier)) {
        auto iter = m_named_consts.find(name.lexeme());
        if (iter == m_named_consts.end()) {
            std::stringstream ss;
            ss << "'" << name.lexeme() << "' is not a defined constant";
            name.error(ss.str());
        }
        return iter->second;
    }
    expect_error("atom");
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
