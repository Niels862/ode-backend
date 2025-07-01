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
        case '+':
        case '-':
        case '*':
        case '/':
        case '>':
        case '=':
            return true;
    }

    return false;
}

static bool is_separator(uint32_t c) {
    switch (c) {
        case '(':
        case ')':
        case '{':
        case '}':
        case '[':
        case ']':
        case ':':
        case ',':
        case ';':
            return true;
    }

    return false;
}

static TokenType classify_token(std::string_view const &lexeme, 
                                TokenType fallback) {
    for (std::size_t i = 0; i < NTokenTypes; i++) {
        TokenType type = static_cast<TokenType>(i);
        if (to_string(type) == lexeme) {
            return type;
        } 
    }
    return fallback;
}

static TokenType classify_keyword(std::string_view const &lexeme) {
    return classify_token(lexeme, TokenType::Identifier);
}

static TokenType classify_operator(std::string_view const &lexeme) {
    return classify_token(lexeme, TokenType::None);
}

static TokenType classify_separator(std::string_view const &lexeme) {
    return classify_token(lexeme, TokenType::None);
}

Lexer::Lexer()
        : m_text{}, m_curr{}, m_curr_pos{}, m_base{}, m_base_pos{} {}

std::vector<Token> Lexer::lex(std::string &filename) {
    read_file(filename);

    while (!at_eof()) {
        set_base();
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

void Lexer::read_file(std::string &filename) {
    std::ifstream file(filename);

    if (!file) {
        throw std::runtime_error("Could not open file: " + filename);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    m_text = buffer.str();
    m_curr_pos = TextPosition(filename);
}

bool Lexer::at_eof() const {
    return m_curr >= m_text.length();
}

uint32_t Lexer::get() const {
    return m_text[m_curr];
}

void Lexer::set_base() {
    m_base = m_curr;
    m_base_pos = m_curr_pos;
}

void Lexer::forward() {
    if (get() == '\n') {
        m_curr_pos.next_line();
    } else {
        m_curr_pos.next_col();
    }

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
    while (get() != '\n' && !at_eof()) {
        forward();
    }
}

std::string_view Lexer::lexeme() const {
    return std::string_view(m_text).substr(m_base, m_curr - m_base);
}

void Lexer::emit(TokenType type) {
    m_tokens.emplace_back(type, lexeme(), m_base_pos);
}
