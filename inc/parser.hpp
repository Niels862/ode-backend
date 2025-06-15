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

    bool open_list();
    bool is_list_end();

    std::unique_ptr<AnalogChip> parse_chip();
    void parse_io_modes(AnalogChip &chip);
    
    void parse_cabs_list(AnalogChip &chip);
    void parse_cab_setup(AnalogChip &chip, AnalogBlock &cab);
    Clock &parse_clock_id(AnalogChip &chip);
    void parse_cab(AnalogBlock &cab);

    double parse_expression();

    int64_t parse_ranged_integer_expression(int64_t lo, int64_t hi, 
                                            std::string ctx);
    int64_t parse_integer_expression();
    double parse_double_expression();

    std::vector<Token> m_tokens;
    std::size_t m_curr;

    std::vector<std::unordered_set<std::string>> m_opened;
    std::vector<std::string> m_opened_names;
};

#endif
