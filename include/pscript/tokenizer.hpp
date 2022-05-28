#pragma once

#include <string>
#include <vector>

namespace ps {

class script;

enum class token_type {
    // identifiers
    none,
    identifier,
    keyword,
    // control tokens
    brace,
    parenthesis,
    semicolon,
    op,
    comma,
    // string, number, integer, etc
    constant
};

struct token {
    std::string str {};
    token_type type = token_type::none;
    // add source location information
};

struct tokenize_result {
    std::vector<ps::token> tokens;
    // maybe add diagnostics
};

ps::tokenize_result tokenize(ps::script const& script);

}