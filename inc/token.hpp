#ifndef OBC_TOKEN_HPP
#define OBC_TOKEN_HPP

#include <string>
#include <string_view>
#include <iostream>

enum class TokenType {
    None,
    Identifier,
    Number,

    Chip,
    With,
    Clocks,
    As,
    Cab,

    Arrow,
    Dash,

    LBrace,
    RBrace,
    LSqBracket,
    RSqBracket,
    Colon,
    Comma,
    
    EndOfFile,
};

char const *to_string(TokenType type);
std::ostream &operator <<(std::ostream &os, TokenType type);

class TextPosition {
public:
    TextPosition()
            : m_filename{""}, m_line{}, m_col{} {}

    TextPosition(std::string_view filename)
            : m_filename{filename}, m_line{1}, m_col{1} {}

    void next_line() { m_line++; m_col = 1; }
    void next_col() { m_col++; }

    friend std::ostream &operator <<(std::ostream &os, 
                                     TextPosition const &token);

private:
    std::string_view m_filename;
    std::size_t m_line, m_col;
};

class Token {
public:
    Token()
            : m_type{TokenType::None}, m_lexeme{""}, m_pos{} {}

    Token(TokenType type, std::string_view lexeme, TextPosition const &pos)
            : m_type{type}, m_lexeme{lexeme}, m_pos{pos} {}

    [[noreturn]] void error(std::string const &s);

    TokenType type() const { return m_type; }
    std::string_view const &lexeme() const { return m_lexeme; }
    TextPosition const &pos() const { return m_pos; }

    operator bool() { 
        return m_type != TokenType::None;
    }

    friend bool operator ==(Token const &token, std::string_view s) {
        return token.m_lexeme == s;
    }

    friend std::ostream &operator <<(std::ostream &os, Token const &token);

private:
    TokenType m_type;
    std::string_view m_lexeme;
    TextPosition m_pos;
};

#endif
