#ifndef OBC_PARSER_HPP
#define OBC_PARSER_CPP

#include "token.hpp"
#include "analog-chip.hpp"
#include <vector>
#include <unordered_map>
#include <unordered_set>

class Parser {
public:
    Parser();

    std::unique_ptr<AnalogChip> parse(std::vector<Token> tokens);

private:
    [[noreturn]] void expect_error(std::string const &expected);
    void check_shadowed_definition(Token &name);

    AnalogModule *find_cam(AnalogChip &chip, Token &name);

    bool at_eof() const;
    Token &accept(TokenType type);
    Token &expect(TokenType type);
    bool matches(TokenType type);
    void forward();

    void open_attribute_map(std::string const &name);
    bool has_next_attribute();
    Token &parse_attribute();
    [[noreturn]] void unknown_attribute(Token &attr);
    void close_attribute_map();

    bool open_list();
    bool is_list_end();

    void parse_let_declaration();

    std::unique_ptr<AnalogChip> parse_chip();
    void parse_io_modes(AnalogChip &chip);
    
    void parse_cabs_list(AnalogChip &chip);
    void parse_cab_setup(AnalogChip &chip, AnalogBlock &cab);
    Clock &parse_clock_id(AnalogChip &chip);
    void parse_cab(AnalogBlock &cab);

    void parse_cam_list(AnalogBlock &cab);
    void parse_cam(AnalogBlock &cab);

    void parse_routing(AnalogChip &chip);
    void parse_routing_entry(AnalogChip &chip);
    OutputPort &parse_output_port(AnalogChip &chip);
    InputPort &parse_input_port(AnalogChip &chip);

    double parse_expression();
    double parse_sum();
    double parse_term();
    double parse_unary();
    double parse_atom();

    int64_t parse_ranged_integer_expression(int64_t lo, int64_t hi, 
                                            std::string ctx);
    int64_t parse_integer_expression();
    double parse_double_expression();

    std::vector<Token> m_tokens;
    std::size_t m_curr;

    std::vector<std::unordered_set<std::string_view>> m_opened;
    std::vector<std::string> m_opened_names;

    std::unordered_map<std::string_view, AnalogModule *> m_chip_cams;
    std::unordered_map<std::string_view, double> m_named_consts;
};

#endif
