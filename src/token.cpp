#include "token.hpp"

char const *to_string(TokenType type) {
    switch (type) {
        case TokenType::None:           return "none";
        case TokenType::Identifier:     return "identifier";
        case TokenType::Number:         return "number";
        case TokenType::EndOfFile:      return "end-of-file";
        case TokenType::Chip:           return "chip";
        case TokenType::With:           return "with";
        case TokenType::As:             return "as";
        case TokenType::Clocks:         return "clocks";
        case TokenType::Io:             return "io";
        case TokenType::Cab:            return "cab";
        case TokenType::Cabs:           return "cabs";
        case TokenType::Cams:           return "cams";
        case TokenType::Routing:        return "routing";
        case TokenType::Arrow:          return "->";
        case TokenType::Dash:           return "-";
        case TokenType::LBrace:         return "{";
        case TokenType::RBrace:         return "}";
        case TokenType::LSqBracket:     return "[";
        case TokenType::RSqBracket:     return "]";
        case TokenType::Colon:          return ":";
        case TokenType::Comma:          return ",";
        default:                        return "";
    }
}

std::ostream &operator <<(std::ostream &os, TokenType type) {
    os << to_string(type);
    return os;
}

std::ostream &operator <<(std::ostream &os, Token const &token) {
    os << "<" << token.m_type << "> '" << token.m_lexeme << "'";

    return os;
}
