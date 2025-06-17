#ifndef OBC_LEXER_HPP
#define OBC_LEXER_HPP

#include "token.hpp"
#include <vector>

class Lexer {
public:
    Lexer();

    std::vector<Token> lex(std::string &filename);

private:
    void read_file(std::string &filename);

    bool at_eof() const;
    uint32_t get() const;
    void set_base();
    void forward();

    void lex_identifier();
    void lex_number();
    void lex_operator();
    void lex_separator();
    void skip_comment();

    std::string_view lexeme() const;

    void emit(TokenType type);

    std::string m_text;

    std::size_t m_curr;
    TextPosition m_curr_pos;

    std::size_t m_base;
    TextPosition m_base_pos;

    std::vector<Token> m_tokens;
};

#endif
