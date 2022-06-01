#include <pscript/context.hpp>

#include <peglib.h>

#include <iostream> // debug

// TODO: proper error reporting

namespace ps {

// Not too proud of this one, but moving to an external file is also not optimal
static const char* grammar = R"(
# --------------------------------
# Explanation of basic PEG syntax:
# --------------------------------
#
# Rules are defined as
# rulename <- match
# To define what matches a rule, items can be sequenced together by simply putting them next to each other.
# myrule <- 'A' 'B'
# will match 'AB' to myrule.
# rules can contain other rules:
# myrule <- rulea ruleb
# rulea <- 'A'
# ruleb <- 'B'
#
# There are several operators that can be used to make more complicated rules.
# The '/' operator is a prioritized choice:
# rule <- rule_a / rule_b
# will match rule_a or rule_b, but prefer rule_a in case both are possible.
#
# Normal regular expression operators such as * (match any amount), + (match at least one), and ? (match zero or one) are also allowed.
# for more info on this exact version of PEG, see https://github.com/yhirose/cpp-peglib and https://bford.info/pub/lang/peg.pdf


# ================= base content =================

script <- content

# content makes up the main part of the AST. It stores the entire file, one 'logical' line at a time.
# There are four types of 'logical' lines.
# - comments - Start with //, simple comment like in any language. Spans the whole line.
# - elements - These are basic statements that can be found inside (or outside) functions.
# - namespace declarations
# - functions - Starts a function declaration.
# - structs - Starts a struct declaration
content <- (comment / element / namespace_decl / function / struct)*

# ================= basic syntactical symbols =================

space <- ' '*
operator <- '<=' / '>=' / '==' / '!=' / '*' / '/' / '+' / '-' / '<' / '>' / '=' / '+=' / '-='
unary_operator <- '-' / '++' / '--' / '!'
assign <- '='
colon <- ':'
quote <- '"'
parens_open <- < '(' >
parens_close <- < ')' >
brace_open <- '{'
brace_close <- '}'
list_open <- '['
list_close <- ']'
arrow <- '->'
dot <- '.'
star <- '*'
comma <- ','
semicolon <- ';'
# todo: find better 'any' than this.
any <- [a-zA-Z0-9.,:;_-+-*/=?!() ]*
# our language ignores whitespace
%whitespace <- [ \n\t\r]*

# ================= identifiers and literals =================

# identifiers can only start with a lower or uppercase letter, and contain letters, numbers and underscores otherwise.
identifier <- ([a-zA-Z] [a-zA-Z_0-9]*)
# a literal is currently either a string or a number.
literal <- string / number
number <- float / integer
integer <- < [0-9]+ >
float <- < [0-9]+.[0-9] >
string <- < quote [a-z]* quote >

# ================= typenames =================

typename <- builtin_type / namespace_list? identifier
# typenames can be prefixed by namespace qualifiers
namespace_list <- (namespace '.')+ { no_ast_opt }
namespace <- identifier
# match builtin types separately for easier interpreting
builtin_type <- 'int' / 'float' / 'str' / 'list'

# ================= namespaces =================

namespace_decl <- 'namespace ' identifier space brace_open content brace_close

# ================= functions =================

# for functions we need to be able to create parameter lists.
parameter_list <- parameter (comma parameter)* { no_ast_opt }
parameter <- identifier colon typename

# a function can either be an externally declared function, or a function definition.
function <- function_ext / function_def

# extern fn my_external_function(param1: typename, param2: typename) -> return_type;
function_ext <- 'extern fn ' identifier parens_open parameter_list? parens_close arrow typename semicolon

# fn my_function(param1: typename, param2: typename) -> return_type { function_body }
function_def <- 'fn ' identifier parens_open parameter_list? parens_close arrow typename space compound

builtin_function <- '__print'

# ================= structs =================

# struct my_struct {
#   a: float;
#   b: int = 0;
# };
struct <- 'struct ' identifier space brace_open struct_items brace_close semicolon
struct_items <- (struct_item semicolon)*
struct_item <- identifier colon typename struct_initializer?
struct_initializer <- assign expression

# basic statement, control structure such as if/while, or a for loop.
element <- comment / statement / if / while / for

# ================= statements =================

# a statement can be
# - an import statement
# - a return statement
# - a variable declaration
# - an expression (usually a call expression)

statement <- statement_base semicolon
statement_base <- import / return / declaration / expression

# ================= import statements =================

# import my_module.function;
# import my_module.*;

import <- 'import ' module
module <- (module_name dot)? (identifier / module_import_all)
module_import_all <- star
module_name <- identifier

# ================= return statements =================

return <- 'return ' expression? { no_ast_opt }

# ================= variable declarations =================

declaration <- 'let ' identifier space assign space expression

# ================= compound statements

compound <- element / (brace_open element* brace_close)

# ================= expressions =================

# There are four kinds of expressions that each need to be parsed differently.
# - a constructor expression in the form MyType{arguments...}
# - a list expression in the form [list_elements...]
# - an 'operator' epxression in the form expression operator expression (ex. a == 8)
# - a call expression in the form my_function(arguments...)
expression <- constructor_expression / list_expression / op_expression / call_expression

# ----- constructor epxression -----
constructor_expression <- identifier space '{' argument_list? '}'

# ----- list expression -----
list_expression <- list_open argument_list? list_close

# ----- operator expression -----
op_expression <- atom (operator atom)* {
    precedence
    L =
    L == != <= >= < >
    L - +
    L / *
}
# this is to fully support recursive expressions.
atom <- unary_operator? (parens_open expression parens_close / call_expression / parens_open operand parens_close / operand)
operand <- < literal / identifier >

# ----- call expression -----
call_expression <- namespace_list? (identifier / builtin_function) space parens_open argument_list? parens_close
argument_list <- argument ( comma argument )* { no_ast_opt }
argument <- expression

# ================= control sequences =================

# ----- if/else statement -----
if <- 'if' parens_open expression parens_close compound else?
else <- 'else' compound

# ----- while statement -----
while <- 'while' parens_open expression parens_close compound

# ----- for statement -----

for <- 'for' parens_open for_content parens_close compound
# note that there are two types of for loops: for-each loops and regular 'manual' for loops.
for_content <- for_manual / for_each
for_manual <- declaration semicolon expression semicolon expression
for_each <- 'let ' identifier space colon space expression

# ================= comment =================

comment <- '//' any '\n'
)";

context::context(std::size_t mem_size) : mem(mem_size) {
    ast_parser = std::make_unique<peg::parser>(grammar);
    ast_parser->enable_ast();
    ast_parser->enable_packrat_parsing();
}

ps::memory_pool& context::memory() noexcept {
    return mem;
}

ps::memory_pool const& context::memory() const noexcept {
    return mem;
}

void context::dump_memory() const noexcept {
    auto print = [this](ps::pointer ptr) {
        auto const v = static_cast<uint8_t>(mem[ptr]);
        printf("%02X", v);
    };

    for (auto it = mem.begin(); it != mem.end(); it += 32) {
        // Print lines of 32 bytes, grouped in blocks of 8 (as this is the smallest possible block size)
        for (int i = 0; i < 32; i += 8) {
            print(it + i);
            print(it + i + 1);
            print(it + i + 2);
            print(it + i + 3);
            printf(" ");
        }
        printf("\n");
    }
}

peg::parser const& context::parser() const noexcept {
    return *ast_parser;
}

ps::variable& context::create_variable(std::string const& name, ps::value&& initializer, block_scope* scope) {
    auto& variables = scope ? scope->local_variables : global_variables;
    if (auto old = variables.find(name); old != variables.end()) {
        // Variable already exists, so shadow it with a new type by assigning a new value to it.
        // We first need to free the old memory
        memory().free(old->second.value().pointer());
        old->second.value() = std::move(initializer);
        return old->second;
    } else {
        auto it = variables.insert({name, ps::variable(name, std::move(initializer))});
        // make sure name string view points to the entry in the map, so it is guaranteed to live as long as the variable lives.
        it.first->second.set_name(it.first->first);
        return it.first->second;
    }
}

ps::value& context::get_variable_value(std::string const& name, block_scope* scope) {
    auto& variables = scope ? scope->local_variables : global_variables;
    auto it = variables.find(name);
    if (it == variables.end()) {
        // if we were looking in local scope, also try parent scope
        if (scope) {
            return get_variable_value(name, scope->parent);
        } else {
            throw std::runtime_error("variable not declared in current scope: " + name);
        }
    }
    else return it->second.value();
}

void context::execute(ps::script const& script) {
    std::shared_ptr<peg::Ast> const& ast = script.ast();
    execute(ast.get(), nullptr); // start execution in global scope
}

ps::value context::execute(peg::Ast const* node, block_scope* scope) {
    // TODO: Rework return value system to put return value in call stack instead!
    if (node_is_type(node, "declaration")) {
        evaluate_declaration(node, scope);
    }

    if (node_is_type(node, "function")) {
        evaluate_function_definition(node);
    }

    ps::value ret = ps::value::null();

    if (node_is_type(node, "call_expression")) {
        return evaluate_function_call(node, scope);
    }

    auto update_ret = [&ret](ps::value&& v) {
        if (ret.is_null()) ret = std::move(v);
    };

#define check_return() if (!call_stack.empty() && call_stack.top().returned) return ret

    if (node_is_type(node, "statement") || node_is_type(node, "compound") || node_is_type(node, "script") || node_is_type(node, "content")) {
        for (auto const& child : node->nodes) {
            update_ret(execute(child.get(), scope));
            check_return();
        }
    }

    if (node_is_type(node, "return")) {
        call_stack.top().returned = true;
        // first child node of a return statement is the return expression.
        if (!node->nodes.empty()) {
            update_ret(evaluate_expression(node->nodes[0].get(), scope));
        }
    }

    if (node_is_type(node, "if")) {
        peg::Ast const* condition_node = find_child_with_type(node, "expression");
        ps::value condition = evaluate_expression(condition_node, scope);
        // If the condition evaluates to true, we can execute the compound block with a new scope
        block_scope local_scope {};
        local_scope.parent = scope;
        if (static_cast<bool>(condition)) {
            peg::Ast const* compound = find_child_with_type(node, "compound");
            update_ret(execute(compound, &local_scope));
            check_return();
        } else {
            // if an else block is present, execute it
            peg::Ast const* else_block = find_child_with_type(node, "else");
            if (else_block) {
                update_ret(execute(find_child_with_type(else_block, "compound"), &local_scope));
                check_return();
            }
        }
    }

#undef check_return

    return ret;
}

peg::Ast const* context::find_child_with_type(peg::Ast const* node, std::string_view type) const noexcept {
    for (auto const& child : node->nodes) {
        if (child->original_name == type || child->name == type) return child.get();
    }
    return nullptr;
}

bool context::node_is_type(peg::Ast const* node, std::string_view type) const noexcept {
    return node->name == type || node->original_name == type;
}

void context::evaluate_declaration(peg::Ast const* node, block_scope* scope) {
    peg::Ast const* identifier = find_child_with_type(node, "identifier");
    peg::Ast const* initializer = find_child_with_type(node, "expression");

    if (!identifier) throw std::runtime_error("[decl] expected identifier");
    if (!initializer) throw std::runtime_error("[decl] expected initializer");

    ps::value init_val = evaluate_expression(initializer, scope);

    ps::variable& var = create_variable(identifier->token_to_string(), std::move(init_val), scope);
}

void context::evaluate_function_definition(peg::Ast const* node) {
    peg::Ast const* identifier = find_child_with_type(node, "identifier");
    peg::Ast const* params = find_child_with_type(node, "parameter_list");

    peg::Ast const* content = find_child_with_type(node, "compound");
    function func {};
    func.node = content;
    if (params) {
        for (auto const& child : params->nodes) {
            if (!node_is_type(child.get(), "parameter")) continue;
            peg::Ast const* param_name = find_child_with_type(child.get(), "identifier");
            // TODO: extract type information
            func.params.push_back(function::parameter{ .name = param_name->token_to_string() });
        }
    }
    // TODO: return type information?
    auto it = functions.insert({identifier->token_to_string(), std::move(func)});
    // set key reference
    it.first->second.name = it.first->first;
}

ps::value context::evaluate_operand(peg::Ast const* node, block_scope* scope) {
    assert(node_is_type(node, "operand"));

    std::string str_repr = node->token_to_string();
    // integer or floating point literal
    if (std::isdigit(str_repr[0])) {
        if (str_repr.find('.') != std::string::npos) {
            return ps::value::from(memory(), node->token_to_number<ps::types::real>());
        } else {
            return ps::value::from(memory(), node->token_to_number<ps::types::integer>());
        }
    }

    // string literal
    if (str_repr[0] == '\"') {
        throw std::runtime_error("[operand] string literals not yet implemented");
    }

    // identifier
    return get_variable_value(str_repr, scope);
}

ps::value context::evaluate_operator(peg::Ast const* lhs, peg::Ast const* op, peg::Ast const* rhs, block_scope* scope) {
    ps::value left = evaluate_expression(lhs, scope);
    ps::value right = evaluate_expression(rhs, scope);

    std::string op_str = op->token_to_string();
    if (op_str == "+") return left + right;
    if (op_str == "*") return left * right;
    if (op_str == "-") return left - right;
    if (op_str == "/") return left / right;
    if (op_str == "==") return left == right;
    if (op_str == "!=") return left != right;
    if (op_str == "<") return left < right;
    if (op_str == ">") return left > right;
    if (op_str == ">=") return left >= right;
    if (op_str == "<=") return left <= right;
    else throw std::runtime_error("[operator] operator " + op_str + " not implemented");
}

std::vector<ps::value> context::evaluate_argument_list(peg::Ast const* call_node, block_scope* scope) {
    peg::Ast const* list = find_child_with_type(call_node, "argument_list");
    if (!list) return {};
    std::vector<ps::value> values {};
    values.reserve(list->nodes.size());
    for (auto const& child :  list->nodes) {
        if (node_is_type(child.get(), "argument")) {
            values.push_back(evaluate_expression(child.get(), scope));
        }
    }
    return values;
}

void context::prepare_function_scope(peg::Ast const* call_node, block_scope* call_scope, function* func, block_scope* func_scope) {
    using namespace std::literals::string_literals;
    func_scope->parent = nullptr; // parent is global scope for function calls (as you can't access variables from previous scope, unlike in if statements).

    auto arguments = evaluate_argument_list(call_node, call_scope);
    // no work
    if (arguments.empty()) return;

    if (arguments.size() != func->params.size()) throw std::runtime_error(
            "[func_call] "s + func->name.data() + ": expected " + std::to_string(func->params.size()) + " arguments, got " + std::to_string(arguments.size()));

    // create variables with function arguments in call scope
    for (size_t i = 0; i < arguments.size(); ++i) {
        ps::variable& _ = create_variable(func->params[i].name, std::move(arguments[i]), func_scope);
    }
}

ps::value context::evaluate_function_call(peg::Ast const* node, block_scope* scope) {
    peg::Ast const* builtin_identifier = find_child_with_type(node, "builtin_function");
    if (builtin_identifier) return evaluate_builtin_function(builtin_identifier->token_to_string(), node, scope);

    peg::Ast const* namespace_identifier = find_child_with_type(node, "namespace_list");
    peg::Ast const* func_identifier_node = find_child_with_type(node, "identifier");

    // namespaced functions are simply stored by concatenating their names together to make the full name, but we won't implement that yet
    // TODO: implement namespaces
    std::string func_name = func_identifier_node->token_to_string();

    auto it = functions.find(func_name);
    if (it == functions.end()) {
        throw std::runtime_error("[func call] function " + func_name + " not found.\n");
    }

    // create function scope for this call
    block_scope local_scope {};

    prepare_function_scope(node, scope, &it->second, &local_scope);
    call_stack.push(function_call {.func = &it->second, .scope = &local_scope, .returned = false});
    ps::value val = execute(it->second.node, &local_scope);
    call_stack.pop();
    return val;
}

ps::value context::evaluate_builtin_function(std::string_view name, peg::Ast const* node, block_scope* scope) {
    auto arguments = evaluate_argument_list(node, scope);
    // builtin function: print
    // TODO: print improvements (format string)
    if (name == "__print") {
        if (arguments.empty()) throw std::runtime_error("[__print()] invalid argument count.");
        ps::value const& to_print = arguments[0];
        visit_value(to_print, [](auto const& val) {
            std::cout << val << std::endl;
        });
        // success
        return ps::value::from(memory(), 0);
    }

    return ps::value::null();
}

ps::value context::evaluate_expression(peg::Ast const* node, block_scope* scope) {
    // base case, an operand is a simple value.
    if (node_is_type(node, "operand")) {
        return evaluate_operand(node, scope);
    }

    // the atom[operand] case was already handled above.
    if (node_is_type(node, "atom")) {
        // skip over children until we find a child node with a type that we need.
        // this is because there are parens_open and parens_close nodes in here
        for (auto const& child : node->nodes) {
            if (node_is_type(child.get(), "expression")) {
                return evaluate_expression(child.get(), scope);
            }
        }
    }

    if (node_is_type(node, "call_expression")) {
        return evaluate_function_call(node, scope);
    }

    if (node_is_type(node, "op_expression")) {
        peg::Ast const* lhs_node = node->nodes[0].get();
        peg::Ast const* operator_node = node->nodes[1].get();
        peg::Ast const* rhs_node = node->nodes[2].get();

        return evaluate_operator(lhs_node, operator_node, rhs_node, scope);
    }

    return ps::value::null();
}

} // namespace ps