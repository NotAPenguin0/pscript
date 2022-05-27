#include <pscript/script.hpp>

#include <pscript/tokenizer.hpp>

namespace ps {

script::script(std::string source) : original_source(std::move(source)) {
    tok_result = ps::tokenize(*this);
}

std::string const& script::source() const {
    return original_source;
}

std::vector<ps::token> const& script::tokens() const {
    return tok_result.tokens;
}

}