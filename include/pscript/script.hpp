#pragma once

#include <string>
#include <memory>

namespace peg {
    class parser;
    struct EmptyType;
    template<typename> struct AstBase;
    using Ast = AstBase<EmptyType>;
}

namespace ps {

class context;

class script {
public:
    // TODO: Add constructor from binary_input_stream maybe?
    explicit script(std::string source, ps::context& ctx);

    /**
     * @brief Get source code of the script
     */
    [[nodiscard]] std::string const& source() const;

    [[nodiscard]] std::shared_ptr<peg::Ast> const& ast() const;

private:
    std::string original_source {};

    std::shared_ptr<peg::Ast> peg_ast {};
};

}