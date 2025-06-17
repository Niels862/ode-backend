#include "token.hpp"
#include <sstream>

char const *to_string(TokenType type) {
    switch (type) {
        case TokenType::None:           return "(none)";
        case TokenType::Identifier:     return "(identifier)";
        case TokenType::Number:         return "(number)";
        case TokenType::EndOfFile:      return "(end-of-file)";
        case TokenType::Let:            return "let";
        case TokenType::Chip:           return "chip";
        case TokenType::With:           return "with";
        case TokenType::Clocks:         return "clocks";
        case TokenType::As:             return "as";
        case TokenType::Cab:            return "cab";
        case TokenType::Cmp:            return "cmp";
        case TokenType::True:           return "true";
        case TokenType::False:          return "false";
        case TokenType::Arrow:          return "->";
        case TokenType::Plus:           return "+";
        case TokenType::Minus:          return "-";
        case TokenType::Asterisk:       return "*";
        case TokenType::Slash:          return "/";
        case TokenType::Equals:         return "=";
        case TokenType::LBracket:       return "(";
        case TokenType::RBracket:       return ")";
        case TokenType::LBrace:         return "{";
        case TokenType::RBrace:         return "}";
        case TokenType::LSqBracket:     return "[";
        case TokenType::RSqBracket:     return "]";
        case TokenType::Colon:          return ":";
        case TokenType::Comma:          return ",";
        case TokenType::Semicolon:      return ";";
        default:                        return "";
    }
}

std::ostream &operator <<(std::ostream &os, TokenType type) {
    os << to_string(type);
    return os;
}

std::ostream &operator <<(std::ostream &os, TextPosition const &pos) {
    os << pos.m_filename << ":" << pos.m_line << ":" << pos.m_col;
    return os;
}

void Token::error(std::string const &s) {
    std::stringstream ss;
    ss << pos() << ": " << s;
    throw std::runtime_error(ss.str());
}

std::ostream &operator <<(std::ostream &os, Token const &token) {
    os << token.m_pos << " <" << token.m_type << "> '" << token.m_lexeme << "'";
    return os;
}
