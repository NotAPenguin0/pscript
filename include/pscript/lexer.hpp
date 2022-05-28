#pragma once

#include <pscript/tokenizer.hpp>

namespace ps {

// lexer will assign token types to each token in the tokenize_result structure.

void lex_tokens(ps::tokenize_result& result);

}