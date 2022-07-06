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

class extern_library;

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

    [[nodiscard]] ps::variable& get_variable(std::string const& name, ps::Ast const* node = nullptr, block_scope* scope = nullptr);
    [[nodiscard]] ps::value& get_variable_value(std::string const& name, ps::Ast const* node = nullptr, block_scope* scope = nullptr);

    /**
     * @brief Executes a script in this context. Note that this function is NOT safe to use in interactive mode, and may be removed in a future version.
     * @param script Script object to execute.
     */
    void execute(ps::script const& script, ps::execution_context exec = {});

    /**
     * @brief Executes a script in this context.
     * @param script Script object to execute.
     */
    void execute(std::shared_ptr<ps::script> const& script, ps::execution_context exec = {});

private:
    struct function {
        // refers to key in map
        std::string_view name;
        ps::Ast const* node = nullptr;

        struct parameter {
            std::string name {};
            ps::type type {};
            std::string type_name {}; // if type is a struct, stores the structs name.
            bool is_variadic = false; // if a parameter is variadic, it will be created as a list<any> under the hood.
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
    // context must keep a list of scripts to make sure function node pointers etc remain valid throughout the entire context
    // lifetime (see for example interactive mode).
    std::vector<std::shared_ptr<ps::script>> executed_scripts;

    ps::value execute(ps::Ast const* node, block_scope* scope, std::string const& namespace_prefix = ""); // namespace prefix used for importing

    static ps::Ast const* find_child_with_type(ps::Ast const* node, unsigned int type) noexcept;

    [[nodiscard]] ps::variable* find_variable(std::string const& name, block_scope* scope);
    void delete_variable(std::string const& name, block_scope* scope);

    // checks both name and original_name
    static bool node_is_type(ps::Ast const* node, unsigned int type) noexcept;

    static ps::type evaluate_type(ps::Ast const* node);
    static std::string evaluate_type_name(ps::Ast const* node);

    void evaluate_declaration(ps::Ast const* node, block_scope* scope);
    void evaluate_function_definition(ps::Ast const* node, std::string const& namespace_prefix = "");
    void evaluate_struct_definition(ps::Ast const* node, std::string const& namespace_prefix = "");
    void evaluate_extern_variable(ps::Ast const* node, std::string const& namespace_prefix = "");

    void evaluate_import(ps::Ast const* node);

    std::vector<ps::value> evaluate_argument_list(ps::Ast const* call_node, block_scope* scope, bool ref = false);

    static std::string parse_namespace(ps::Ast const* node);

    // clears variables in scope, then creates variables for arguments.
    void prepare_function_scope(ps::Ast const* call_node, block_scope* call_scope, function* func, block_scope* func_scope);

    ps::value evaluate_function_call(ps::Ast const* node, block_scope* scope);
    ps::value evaluate_external_call(ps::Ast const* node, block_scope* scope, std::string const& name);
    ps::value evaluate_builtin_function(std::string_view name, ps::Ast const* node, block_scope* scope);
    ps::value evaluate_list_member_function(std::string_view name, ps::variable& object, ps::Ast const* node, block_scope* scope);
    ps::value evaluate_string_member_function(std::string_view name, ps::variable& object, ps::Ast const* node, block_scope* scope);

    // return reference to list value, given index-expr node.
    ps::value& index_list(ps::Ast const* node, block_scope* scope);
    ps::value& access_member(ps::Ast const* node, block_scope* scope);

    ps::value evaluate_operand(ps::Ast const* node, block_scope* scope, bool ref = false);
    ps::value evaluate_operator(ps::Ast const* lhs, ps::Ast const* op, ps::Ast const* rhs, block_scope* scope);
    ps::value evaluate_expression(ps::Ast const* node, block_scope* scope, bool ref = false);
    ps::value evaluate_constructor_expression(ps::Ast const* node, block_scope* scope);
    ps::value evaluate_list(ps::Ast const* node, block_scope* scope);

    // returns true if cast was successful, false otherwise
    static bool try_cast(ps::value& val, ps::type from, ps::type to);

    static void report_error(ps::Ast const* node, std::string_view message) ;
};


/**
 * @brief Interface class for external functions
 */
class extern_library {
public:
    template<typename C>
    void add_function(ps::context& ctx, std::string const& name, C&& callable) {
        functions.insert({name, plib::make_concrete_function<ps::value>(callable, [&ctx](auto x){
            return ps::value::from(ctx.memory(), x);
        })});
    }

    void add_variable(std::string const& name, void* ptr) {
        variables.insert({name, ptr});
    }

    virtual plib::erased_function<ps::value>* get_function(std::string const& name) {
        auto it = functions.find(name);
        if (it != functions.end()) return it->second;
        return nullptr;
    }

    virtual void* get_variable(std::string const& name) {
        auto it = variables.find(name);
        if (it != variables.end()) return it->second;
        return nullptr;
    }

    virtual inline ~extern_library() {
        for (auto& [k, v] : functions) {
            delete v;
        }
        functions.clear();
    }

    std::unique_ptr<extern_library> next = nullptr;

private:
    std::unordered_map<std::string, plib::erased_function<ps::value>*> functions {};
    std::unordered_map<std::string, void*> variables;
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

}
