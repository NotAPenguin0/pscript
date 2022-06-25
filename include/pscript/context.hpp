#pragma once

#include <pscript/memory.hpp>
#include <pscript/variable.hpp>
#include <pscript/script.hpp>

#include <plib/erased_function.hpp>

#include <string>
#include <unordered_map>
#include <optional>
#include <stack>

#include <peglib.h>

namespace peg {
    class parser;
}

namespace ps {

/**
 * @brief Interface class for external functions
 */
class extern_library {
public:
    virtual plib::erased_function<ps::value>* get_function(std::string const& name) = 0;
    virtual void* get_variable(std::string const& name) = 0;

    virtual ~extern_library() = default;

    std::unique_ptr<extern_library> next = nullptr;
};

struct extern_library_chain_builder {
private:
    extern_library* cur_node = nullptr;
    std::unique_ptr<extern_library> lib;
public:
    extern_library_chain_builder& add(std::unique_ptr<extern_library>&& library) {
        if (lib == nullptr) {
            lib = std::move(library);
            cur_node = lib.get();
        } else {
            cur_node->next = std::move(library);
            cur_node = cur_node->next.get();
        }
        return *this;
    }

    std::unique_ptr<extern_library> get() {
        return std::move(lib);
    }
};

struct execution_context {
    std::istream* in = &std::cin;
    std::ostream* out = &std::cout;
    std::ostream* err = &std::cerr;
    extern_library* externs = nullptr;
    std::vector<std::string> module_paths = { "pscript-modules/" };
};

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
     * @brief Dumps memory to exec_ctx.out
     */
    [[maybe_unused]] void dump_memory() const noexcept;

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

    [[nodiscard]] ps::variable& get_variable(std::string const& name, peg::Ast const* node = nullptr, block_scope* scope = nullptr);
    [[nodiscard]] ps::value& get_variable_value(std::string const& name, peg::Ast const* node = nullptr, block_scope* scope = nullptr);

    /**
     * @brief Executes a script in this context.
     * @param script Script object to execute.
     */
    void execute(ps::script const& script, ps::execution_context exec = {});

private:
    struct function {
        // refers to key in map
        std::string_view name;
        peg::Ast const* node = nullptr;

        struct parameter {
            std::string name {};
            ps::type type {};
            std::string type_name {}; // if type is a struct, stores the structs name.
        };
        // parameters this function was declared with
        std::vector<parameter> params;

        ps::type return_type;
        std::string return_type_name {}; // if type is a struct, stores the structs name.
    };

    struct struct_description {
        // refers to key in map
        std::string_view name;

        struct member {
            std::string name;
            ps::value default_value;
            ps::type type {};
            std::string type_name {}; // if type is a struct, stores the structs name.
        };

        std::vector<member> members;
    };

    struct function_call {
        function* func = nullptr;
        block_scope* scope = nullptr;
        std::optional<ps::value> return_val = std::nullopt;
    };

    ps::memory_pool mem;
    std::unordered_map<std::string, ps::variable> global_variables;
    std::unordered_map<std::string, function> functions;
    std::unordered_map<std::string, struct_description> structs;

    struct import_data {
        std::string filepath;
        ps::script script;
    };
    std::vector<import_data> imported_scripts {};
    ps::execution_context exec_ctx;

    std::stack<function_call> call_stack {};

    std::unique_ptr<peg::parser> ast_parser;

    ps::value execute(peg::Ast const* node, block_scope* scope, std::string const& namespace_prefix = ""); // namespace prefix used for importing

    static peg::Ast const* find_child_with_type(peg::Ast const* node, std::string_view type) noexcept;

    [[nodiscard]] ps::variable* find_variable(std::string const& name, block_scope* scope);

    // checks both name and original_name
    static bool node_is_type(peg::Ast const* node, std::string_view type) noexcept;

    static ps::type evaluate_type(peg::Ast const* node);
    static std::string evaluate_type_name(peg::Ast const* node);

    void evaluate_declaration(peg::Ast const* node, block_scope* scope);
    void evaluate_function_definition(peg::Ast const* node, std::string const& namespace_prefix = "");
    void evaluate_struct_definition(peg::Ast const* node, std::string const& namespace_prefix = "");
    void evaluate_extern_variable(peg::Ast const* node, std::string const& namespace_prefix = "");

    void evaluate_import(peg::Ast const* node);

    std::vector<ps::value> evaluate_argument_list(peg::Ast const* call_node, block_scope* scope, bool ref = false);

    static std::string parse_namespace(peg::Ast const* node);

    // clears variables in scope, then creates variables for arguments.
    void prepare_function_scope(peg::Ast const* call_node, block_scope* call_scope, function* func, block_scope* func_scope);

    ps::value evaluate_function_call(peg::Ast const* node, block_scope* scope);
    ps::value evaluate_external_call(peg::Ast const* node, block_scope* scope, std::string const& name);
    ps::value evaluate_builtin_function(std::string_view name, peg::Ast const* node, block_scope* scope);
    ps::value evaluate_list_member_function(std::string_view name, ps::variable& object, peg::Ast const* node, block_scope* scope);
    ps::value evaluate_string_member_function(std::string_view name, ps::variable& object, peg::Ast const* node, block_scope* scope);

    // return reference to list value, given index-expr node.
    ps::value& index_list(peg::Ast const* node, block_scope* scope);
    ps::value& access_member(peg::Ast const* node, block_scope* scope);

    ps::value evaluate_operand(peg::Ast const* node, block_scope* scope, bool ref = false);
    ps::value evaluate_operator(peg::Ast const* lhs, peg::Ast const* op, peg::Ast const* rhs, block_scope* scope);
    ps::value evaluate_expression(peg::Ast const* node, block_scope* scope, bool ref = false);
    ps::value evaluate_constructor_expression(peg::Ast const* node, block_scope* scope);
    ps::value evaluate_list(peg::Ast const* node, block_scope* scope);

    // returns true if cast was successful, false otherwise
    bool try_cast(ps::value& val, ps::type from, ps::type to);

    static void report_error(peg::Ast const* node, std::string_view message) ;
};

}