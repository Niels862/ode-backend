#include "lexer.hpp"
#include <fstream>
#include <sstream>
#include <cctype>
#include <stdexcept>
#include <iostream>
#include <unordered_map>

static bool is_whitespace(uint32_t c) {
    return std::isspace(c) || c == '\n' || c == '\r' || c == '\t';
}

static bool is_operator(uint32_t c) {
    switch (c) {
        case '-':
        case '>':
            return true;
    }

    return false;
}

static TokenType classify_keyword(std::string const &lexeme) {
    static const std::unordered_map<std::string, TokenType> keywords {
        { "chip",       TokenType::Chip },
        { "with",       TokenType::With },
        { "as",         TokenType::As },
        { "clocks",     TokenType::Clocks },
        { "io",         TokenType::Io },
        { "cab",        TokenType::Cab },
        { "cabs",       TokenType::Cabs },
        { "cams",       TokenType::Cams },
        { "routing",    TokenType::Routing }
    };

    auto iter = keywords.find(lexeme);
    if (iter == keywords.end()) {
        return TokenType::Identifier;
    }
    return iter->second;
}

static TokenType classify_operator(std::string const &lexeme) {
    static const std::unordered_map<std::string, TokenType> operators {
        { "->",         TokenType::Arrow },
        { "-",          TokenType::Dash }
    };

    auto iter = operators.find(lexeme);
    if (iter == operators.end()) {
        return TokenType::None;
    }
    return iter->second;
}

static TokenType classify_separator(std::string const &lexeme) {
    static const std::unordered_map<std::string, TokenType> keywords {
        { "{",          TokenType::LBrace },
        { "}",          TokenType::RBrace },
        { "[",          TokenType::LSqBracket },
        { "]",          TokenType::RSqBracket },
        { ":",          TokenType::Colon },
        { ",",          TokenType::Comma },
    };

    auto iter = keywords.find(lexeme);
    if (iter == keywords.end()) {
        return TokenType::None;
    }
    return iter->second;
}

static bool is_separator(uint32_t c) {
    switch (c) {
        case '{':
        case '}':
        case '[':
        case ']':
        case ':':
        case ',':
            return true;
    }

    return false;
}

Lexer::Lexer()
        : m_text{}, m_curr{}, m_base{} {}

std::vector<Token> Lexer::lex(std::string const &filename) {
    read_file(filename);

    while (!at_eof()) {
        m_base = m_curr;
        uint32_t c = get();

        if (std::isalpha(c) || c == '_') {
            lex_identifier();
        } else if (std::isdigit(c) || c == '.') {
            lex_number();
        } else if (is_operator(c)) {
            lex_operator();
        } else if (is_separator(c)) {
            lex_separator();
        } else if (c == '#') {
            skip_comment();
        } else if (is_whitespace(c)) {
            forward();
        } else {
            std::stringstream ss;
            ss << "unrecognized character: '" << std::string(1, c) << "'";
            throw std::runtime_error(ss.str());
        }
    }

    emit(TokenType::EndOfFile);
    return m_tokens;
}

void Lexer::read_file(std::string const &filename) {
    std::ifstream file(filename);

    if (!file) {
        throw std::runtime_error("Could not open file: " + filename);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    m_text = buffer.str();

    std::cout << m_text << std::endl;
}

bool Lexer::at_eof() const {
    return m_curr >= m_text.length();
}

uint32_t Lexer::get() const {
    return m_text[m_curr];
}

void Lexer::forward() {
    if (!at_eof()) {
        m_curr++;
    }
}

void Lexer::lex_identifier() {
    uint32_t c = get();

    while (std::isalnum(c) || c == '_') {
        forward();
        c = get();
    }

    emit(classify_keyword(lexeme()));
}

void Lexer::lex_number() {
    uint32_t c = get();

    while (std::isdigit(c) || c == '.') {
        forward();
        c = get();
    }

    emit(TokenType::Number);
}

void Lexer::lex_operator() {
    uint32_t c = get();

    while (is_operator(c)) {
        forward();
        c = get();
    }

    TokenType type = classify_operator(lexeme());
    if (type == TokenType::None) {
        std::stringstream ss;
        ss << "not an operator: '" << lexeme() << "'"; 
        throw std::runtime_error(ss.str());
    }

    emit(type);
}

void Lexer::lex_separator() {
    forward();

    TokenType type = classify_separator(lexeme());
    if (type == TokenType::None) {
        std::stringstream ss;
        ss << "not a separator: '" << lexeme() << "'"; 
        throw std::runtime_error(ss.str());
    }

    emit(type);
}

void Lexer::skip_comment() {
    forward();
}

std::string Lexer::lexeme() const {
    return m_text.substr(m_base, m_curr - m_base);
}

void Lexer::emit(TokenType type) {
    m_tokens.emplace_back(type, lexeme());
}
