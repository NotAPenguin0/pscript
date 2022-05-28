#pragma once

// This file contains lists with syntax elements like keywords and operators.

#include <string_view>
#include <array>

namespace ps::syntax {

using namespace std::literals::string_view_literals;

constexpr char quote = '\"';
constexpr char brace_open = '{';
constexpr char brace_close = '}';
constexpr char parens_open = '(';
constexpr char parens_close = ')';
constexpr char comma = ',';
constexpr char semicolon = ';';

constexpr std::array keywords {"let"sv, "if"sv, "fn"sv};

constexpr std::array operators {"="sv, "+"sv, "-"sv, "*"sv, "/"sv, "=="sv};

}