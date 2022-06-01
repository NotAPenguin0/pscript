#pragma once

#include <pscript/memory.hpp>
#include <pscript/variable.hpp>
#include <pscript/script.hpp>

#include <string>
#include <unordered_map>
#include <optional>
#include <stack>

namespace peg {
    class parser;
}

namespace ps {

// TODO: attach output stream to context instead of hardcoding output to stdout.

/**
 * @brief Core context class for pscript
 */
class context {
public:

    /**
     * @brief Create a context.
     * @param mem_size Initial size of memory (in bytes).
     */
    explicit context(std::size_t mem_size);

    /**
     * @brief Get access to the context's memory pool.
     * @return Reference to the memory pool.
     */
    [[nodiscard]] ps::memory_pool& memory() noexcept;

    /**
     * @brief Get read-only access to the context's memory pool.
     * @return Const reference to the memory pool.
     */
    [[nodiscard]] ps::memory_pool const& memory() const noexcept;

    /**
     * @brief Dumps memory to stdout
     */
    void dump_memory() const noexcept;

    /**
     * @brief Get a reference to the parser object used for parsing scripts.
     * @return Const reference to a peg::parser.
     */
    [[nodiscard]] peg::parser const& parser() const noexcept;

    struct block_scope {
        block_scope* parent = nullptr;
        std::unordered_map<std::string, ps::variable> local_variables;
    };

    /**
     * @brief Creates a new variable with an initializer.
     * @throws std::runtime_error on failure.
     * @tparam T Must be a valid initializer type (a type that can be converted to a pscript type.
     * @param name Name of the variable. If this is the name of a previously created variable it will be overwritten
     * @param initializer Value to initialize the variable with.
     * @param scope Local variable scope, or nullptr for global.
     * @return Reference to the created variable.
     */
    template<typename T>
    [[nodiscard]] ps::variable& create_variable(std::string const& name, T const& initializer, block_scope* scope = nullptr) {
        return create_variable(name, ps::value::from(memory(), initializer), scope);
    }

    [[nodiscard]] ps::variable& create_variable(std::string const& name, ps::value&& initializer, block_scope* scope = nullptr);

    [[nodiscard]] ps::value& get_variable_value(std::string const& name, block_scope* scope = nullptr);

    /**
     * @brief Executes a script in this context.
     * @param script Script object to execute.
     */
    void execute(ps::script const& script);

private:
    struct function {
        // refers to key in map
        std::string_view name;
        peg::Ast const* node = nullptr;

        struct parameter {
            std::string name = "";
            ps::value::type type {};
        };
        // parameters this function was declared with
        std::vector<parameter> params;
    };

    struct function_call {
        function* func = nullptr;
        block_scope* scope = nullptr;
        bool returned = false;
    };

    ps::memory_pool mem;
    std::unordered_map<std::string, ps::variable> global_variables;
    std::unordered_map<std::string, function> functions;

    std::stack<function_call> call_stack {};

    std::unique_ptr<peg::parser> ast_parser;

    ps::value execute(peg::Ast const* node, block_scope* scope);

    peg::Ast const* find_child_with_type(peg::Ast const* node, std::string_view type) const noexcept;

    // checks both name and original_name
    bool node_is_type(peg::Ast const* node, std::string_view type) const noexcept;

    void evaluate_declaration(peg::Ast const* node, block_scope* scope);
    void evaluate_function_definition(peg::Ast const* node);

    std::vector<ps::value> evaluate_argument_list(peg::Ast const* call_node, block_scope* scope);

    // clears variables in scope, then creates variables for arguments.
    void prepare_function_scope(peg::Ast const* call_node, block_scope* call_scope, function* func, block_scope* func_scope);

    ps::value evaluate_function_call(peg::Ast const* node, block_scope* scope);
    ps::value evaluate_builtin_function(std::string_view name, peg::Ast const* node, block_scope* scope);

    ps::value evaluate_operand(peg::Ast const* node, block_scope* scope);
    ps::value evaluate_operator(peg::Ast const* lhs, peg::Ast const* op, peg::Ast const* rhs, block_scope* scope);
    ps::value evaluate_expression(peg::Ast const* node, block_scope* scope);
};

}