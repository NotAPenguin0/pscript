#include <pscript/lexer.hpp>
#include <pscript/syntax.hpp>

#include <algorithm>
#include <optional>

namespace ps {

template<typename C, typename T>
static bool contains(C const& container, T const& value) {
    return std::any_of(std::begin(container), std::end(container),
        [&value](auto const& v) -> bool {
            return value == v;
        });
}

static ps::token_type get_char_token_type(char c) {
    switch(c) {
        case ps::syntax::brace_open:
        case ps::syntax::brace_close:
            return ps::token_type::brace;
        case ps::syntax::parens_open:
        case ps::syntax::parens_close:
            return ps::token_type::parenthesis;
        case ps::syntax::semicolon:
            return ps::token_type::semicolon;
        case ps::syntax::comma:
            return ps::token_type::comma;
        default:
            return ps::token_type::none;
    }
}

static ps::token_type get_token_type(ps::token const& token) {
    auto const& str = token.str;

    if (str.size() == 1) {
        ps::token_type possible_type = get_char_token_type(str[0]);
        // Function maybe returned a token type (maybe it returned none, nothing was found).
        // In this case we found the token type.
        if (possible_type != ps::token_type::none) return possible_type;
    }

    if (contains(syntax::keywords, str)) {
        return ps::token_type::keyword;
    }

    if (contains(syntax::operators, str)) {
        return ps::token_type::op;
    }

    // if the first character in the token is a quote, this token is a string.
    if (str[0] == ps::syntax::quote) {
        return ps::token_type::constant;
    }

    // if the first character is a digit, this token is an integer or floating point
    // number. In both cases we can simply return the 'constant' token type.
    if (std::isdigit(str[0])) {
        return ps::token_type::constant;
    }

    // All other cases are identifiers.
    return ps::token_type::identifier;
}

void lex_tokens(ps::tokenize_result& result) {
    auto& tokens = result.tokens;
    for (auto & token : tokens) {
        token.type = get_token_type(token);
    }
}

}