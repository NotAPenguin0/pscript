#include <pscript/context.hpp>

#include <peglib.h>

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
parens_open <- '('
parens_close <- ')'
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
any <- [a-zA-Z0-9.,:;_-+-*/=?! ]*
# our language ignores whitespace
%whitespace <- [ \n\t\r]*

# ================= identifiers and literals =================

# identifiers can only start with a lower or uppercase letter, and contain letters, numbers and underscores otherwise.
identifier <- [a-zA-Z] [a-zA-Z_0-9]*
# a literal is currently either a string or a number.
literal <- string / number
number <- float / integer
integer <- < [0-9]+ >
float <- < [0-9]+.[0-9] >
string <- < quote [a-z]* quote >

# ================= typenames =================

typename <- builtin_type / namespace_list? identifier
# typenames can be prefixed by namespace qualifiers
namespace_list <- (namespace '.')+
namespace <- identifier
# match builtin types separately for easier interpreting
builtin_type <- 'int' / 'float' / 'str' / 'list'

# ================= namespaces =================

namespace_decl <- 'namespace ' identifier space brace_open content brace_close

# ================= functions =================

# for functions we need to be able to create parameter lists.
parameter_list <- parameter (comma parameter)*
parameter <- identifier colon typename

# a function can either be an externally declared function, or a function definition.
function <- function_ext / function_def

# extern fn my_external_function(param1: typename, param2: typename) -> return_type;
function_ext <- 'extern fn ' identifier parens_open parameter_list? parens_close arrow typename semicolon

# fn my_function(param1: typename, param2: typename) -> return_type { function_body }
function_def <- 'fn ' identifier parens_open parameter_list? parens_close arrow typename space compound

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
element <- statement / if / while / for

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

return <- 'return ' expression?

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
call_expression <- identifier space parens_open argument_list? parens_close
argument_list <- argument ( comma argument )*
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
}

ps::memory_pool& context::memory() noexcept {
    return mem;
}

ps::memory_pool const& context::memory() const noexcept {
    return mem;
}

peg::parser const& context::parser() const noexcept {
    return *ast_parser;
}

void context::execute(ps::script const& script) {

}

} // namespace ps