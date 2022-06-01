#include <pscript/script.hpp>

#include <pscript/context.hpp>
#include <peglib.h>

namespace ps {

script::script(std::string source, ps::context& ctx) : original_source(std::move(source)) {
    // Parse script into its AST.
    peg::parser const& parser = ctx.parser();
    parser.parse(original_source, peg_ast);
    peg_ast = parser.optimize_ast(peg_ast);
}

std::string const& script::source() const {
    return original_source;
}

std::shared_ptr<peg::Ast> const& script::ast() const {
    return peg_ast;
}


}