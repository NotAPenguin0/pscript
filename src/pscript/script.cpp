#include <pscript/script.hpp>

#include <pscript/tokenizer.hpp>
#include <pscript/lexer.hpp>

namespace ps {

script::script(std::string source) : original_source(std::move(source)) {
    tok_result = ps::tokenize(*this);
    ps::lex_tokens(tok_result);
}

std::string const& script::source() const {
    return original_source;
}

std::vector<ps::token> const& script::tokens() const {
    return tok_result.tokens;
}

}