#pragma once

#include <vector>
#include <variant>

#include <string_view>

namespace ps {

enum class ast_node_type {
    script, // used as root node. May have metadata associated with it in the future.
    compound, // compound statement, a block enclosed in {}.
    declaration, // a declaration. Could be a variable or function.
};

namespace ast_nodes {
    struct script {

    };

    struct compound {

    };

    struct expression {

    };

    struct identifier {
        std::string_view name;
    };

    struct declaration {

    };
}

# define T(x) ::ps::ast_nodes::x
# define node_type_list T(script), T(compound), T(declaration), T(expression), T(identifier)

struct ast_node {
    std::variant<node_type_list> data {};
    std::vector<ast_node> children {};

    // add source_location info here
};

#undef T
#undef node_type_list

class ast {
public:

private:
    ast_node root {};
};

}