#ifndef OBC_PARSER_HPP
#define OBC_PARSER_CPP

#include "token.hpp"
#include "analog-chip.hpp"
#include <vector>
#include <unordered_set>

class Parser {
public:
    Parser();

    std::unique_ptr<AnalogChip> parse(std::vector<Token> tokens);

private:
    [[noreturn]] void expect_error(std::string const &expected, 
                                   std::string const &got);
    [[noreturn]] void expect_error(std::string const &expected);

    bool at_eof() const;
    Token &accept(TokenType type);
    Token &expect(TokenType type);
    bool matches(TokenType type);
    void forward();

    void open_attribute_map(std::string const &name);
    bool has_next_attribute();
    std::string parse_attribute();
    [[noreturn]] void unknown_attribute(std::string const &attr);
    void close_attribute_map();

    std::unique_ptr<AnalogChip> parse_chip();
    void parse_io_modes(AnalogChip &chip);

    std::vector<Token> m_tokens;
    std::size_t m_curr;

    std::vector<std::unordered_set<std::string>> m_opened;
    std::vector<std::string> m_opened_names;
};

#endif
