#ifndef OBC_TOKEN_HPP
#define OBC_TOKEN_HPP

#include <string>
#include <iostream>

enum class TokenType {
    None,
    Identifier,
    Number,

    Chip,
    With,
    As,
    Clocks,
    Io,
    Cab,
    Cabs,
    Cams,
    Routing,

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

class Token {
public:
    Token(TokenType type, std::string lexeme)
            : m_type{type}, m_lexeme{lexeme} {}

    TokenType type() const { return m_type; }
    std::string const &lexeme() const { return m_lexeme; }

    friend std::ostream &operator <<(std::ostream &os, Token const &token);

private:
    TokenType m_type;
    std::string m_lexeme;
};

#endif
