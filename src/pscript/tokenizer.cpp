#include <pscript/tokenizer.hpp>
#include <pscript/script.hpp>

#include <cstdlib>

namespace ps {

static bool is_identifier_character(char c) {
    if (c == '_') return true;
    else if (c >= 'a' && c <= 'z') return true;
    else if (c >= 'A' && c <= 'Z') return true;
    else if (c >= '0' && c <= '9') return true;
    else return false;
}

static ps::token token_from_range(std::string const& source, std::size_t start, std::size_t end) {
    return ps::token {
        .str = source.substr(start, end - start)
    };
}

// read token that is either an identifier or a keyword (we can treat these the same).
static ps::token read_identifier_token(std::string const& source, std::size_t& pos) {
    std::size_t start = pos;
    while(pos < source.size() && is_identifier_character(source[pos])) {
        pos += 1;
    }

    return token_from_range(source, start, pos);
}

static ps::token read_numerical_token(std::string const& source, std::size_t& pos) {
    std::size_t start = pos;
    while(pos < source.size() && (std::isdigit(source[pos]) || source[pos] == '.')) {
        pos += 1;
    }

    return token_from_range(source, start, pos);
}

static ps::token read_string_token(std::string const& source, std::size_t& pos) {
    std::size_t start = pos;
    pos += 1; // Increase by one so we don't end on the starting quote.
    while(pos < source.size() && source[pos] != '\"') {
        pos += 1;
    }
    pos += 1; // Include end quote

    return token_from_range(source, start, pos);
}

static ps::token read_control_token(std::string const& source, std::size_t& pos) {
    std::size_t start = pos;

    // Control tokens that are always a single character
    if (source[start] == ';'
        || source[start] == '('
        || source[start] == ')')  {
        pos += 1;
        return token_from_range(source, start, start + 1);
    }


    while(pos < source.size() && !std::isspace(source[pos]) && !std::isalnum(source[pos])) {
        pos += 1;
    }

    return token_from_range(source, start, pos);
}

// Reads the next token and updates the position in the string to the character right after this token.
static ps::token next_token(std::string const& source, std::size_t& pos) {
    // Skip all whitespace
    while(pos < source.size() && std::isspace(source[pos])) {
        pos += 1;
    }

    const char first = source[pos];

    // if the first character is a double quote, we found a string.
    // in this case we read until the next double quote.
    if (first == '\"') {
        return read_string_token(source, pos);
    }

    // If the first character is a letter, this token could be
    // - an identifier
    // - a keyword

    // for both these cases, we need to read until the next character that is not allowed in identifiers.
    // this means that we read until we find a character that is not a letter, digit or underscore

    if (std::isalpha(first)) {
        return read_identifier_token(source, pos);
    }

    // if the first character is a digit, we found a numerical value.
    // in this case we read until we find a character that is not a digit or a dot.
    if (std::isdigit(first)) {
        return read_numerical_token(source, pos);
    }

    // if the first character is any other character, this token is a special character (like an operator, or a semicolon).
    // in this case we read until the next whitespace and treat the result as a token.
    return read_control_token(source, pos);
}

ps::tokenize_result tokenize(ps::script const& script) {
    std::string const& source = script.source();
    ps::tokenize_result result = {};

    // Current position in source string
    std::size_t pos = 0;

    while(pos < source.size()) {
        result.tokens.push_back(next_token(source, pos));
    }

    return result;
}

}