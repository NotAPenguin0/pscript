#include <pscript/context.hpp>
#include <algorithm>
#include <iostream>

#include <pscript/tokenizer.hpp>
#include <peglib.h> // usually not necessary, but we add this here so we can print the AST as a debug operation.

std::ostream& operator<<(std::ostream& out, ps::token_type const& type) {
#define gen(name) if (type == ps::token_type:: name) return out << #name ;
    gen(none)
    gen(identifier)
    gen(keyword)
    gen(brace)
    gen(parenthesis)
    gen(semicolon)
    gen(comma)
    gen(op)
    gen(constant)
#undef gen
    return out;
}

std::ostream& operator<<(std::ostream& out, ps::token const& tok) {
    return out << tok.str << '(' << tok.type << ')';
}

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>
#include <catch2/matchers/catch_matchers_predicate.hpp>
#include <catch2/matchers/catch_matchers_templated.hpp>

using Catch::Matchers::Equals;
using Catch::Matchers::Predicate;

template<typename T>
bool range_equal(ps::memory_pool const& memory, ps::pointer begin, ps::pointer end, T const& val) {
    for (ps::pointer it = begin; it < end; it += sizeof(T)) {
        T const& v = memory.get<T>(it);
        if (v != val) return false;
    }
    return true;
}

struct TokenEqualMatcher : Catch::Matchers::MatcherGenericBase {
    explicit TokenEqualMatcher(std::vector<std::string> const& tokens)
    : tokens{ tokens } {

    }

    bool match(std::vector<ps::token> const& other) const {
        return std::equal(tokens.begin(), tokens.end(),
                          other.begin(), other.end(),
                          [](std::string const& a, ps::token const& b) {
                                     return a == b.str;
                                }
        );
    }

    std::string describe() const override {
        return "Token strings equal: " + Catch::rangeToString(tokens);
    }

private:
    std::vector<std::string> const& tokens;
};

struct TokenTypeMatcher : Catch::Matchers::MatcherGenericBase {
    explicit TokenTypeMatcher(std::vector<ps::token_type> const& tokens) : tokens{tokens} {

    }

    bool match(std::vector<ps::token> const& other) const {
        return std::equal(tokens.begin(), tokens.end(),
                          other.begin(), other.end(),
                          [](ps::token_type a, ps::token const& b) {
                              return a == b.type;
                          }
        );
    }

    std::string describe() const override {
        return "Token types equal: " + Catch::rangeToString(tokens);
    }

private:
    std::vector<ps::token_type> const& tokens;
};

TEST_CASE("pscript context", "[context]") {
    // create context with 1 MiB memory.
    constexpr std::size_t memsize = 1024 * 1024;
    ps::context ctx(memsize);

    // Verify memory was properly allocated
    REQUIRE(ctx.memory().size() == memsize);

    SECTION("memory access") {
        ps::memory_pool const& memory = ctx.memory();
        ps::pointer p = 0;

        CHECK(memory.verify_pointer(p));
        // Middle of address range
        CHECK(memory.verify_pointer(p + memsize / 2));
        // This is out of bounds
        CHECK(!memory.verify_pointer(p + memsize));
        // Null pointer can't be valid either
        CHECK(!memory.verify_pointer(ps::null_pointer));

        // Verify that memory is zeroed out on construction
        CHECK(range_equal(memory, memory.begin(), memory.end(), ps::byte{ 0x00 }));
    }

    SECTION("memory allocation") {
        ps::memory_pool& memory = ctx.memory();

        // Try to allocate some memory and verify pointers
        ps::pointer p0 = memory.allocate(10);
        ps::pointer p1 = memory.allocate(2);
        ps::pointer p2 = memory.allocate(1000);

        CHECK(memory.verify_pointer(p0));
        CHECK(memory.verify_pointer(p1));
        CHECK(memory.verify_pointer(p2));

        // Impossible to allocate
        ps::pointer p3 = memory.allocate(memsize + 1000);
        CHECK(p3 == ps::null_pointer);

        memory.free(p0);
        memory.free(p1);
        memory.free(p2);
    }

    SECTION("variables") {
        ps::memory_pool& memory = ctx.memory();

        ps::variable& x = ctx.create_variable("x", 5);
        CHECK(x.value().int_value() == 5);

        // Variable shadowing
        ps::variable& x_float = ctx.create_variable("x", 3.14f);
        CHECK(x.value().real_value() == 3.14f);
    }
}

TEST_CASE("script expression parser", "[script]") {
    constexpr std::size_t memsize = 128;
    ps::context ctx(memsize);

    SECTION("operands") {
        std::string source = R"(
            let x = 5;
            let y = x;
            let z = 4.4;
        )";

        ps::script script(source, ctx);
        ctx.execute(script);

        ps::value& y = ctx.get_variable_value("y");
        ps::value& z = ctx.get_variable_value("z");
        CHECK(y.int_value() == 5);
        CHECK(z.real_value() == 4.4f);
    }

    SECTION("basic operations") {
        std::string source = R"(
            let x = 3 + 2;
            let y = x + 5;
            let z = x * x;
        )";

        ps::script script(source, ctx);
        ctx.execute(script);

        CHECK(ctx.get_variable_value("x").int_value() == 5);
        CHECK(ctx.get_variable_value("y").int_value() == 10);
        CHECK(ctx.get_variable_value("z").int_value() == 25);
    }

    SECTION("parentheses and precedence") {
        std::string source = R"(
            let x = 2 * (3 + 2);
            let y = 2 * 3 + 2;
            let z = 2 + 2 * 3;
        )";

        ps::script script(source, ctx);
        ctx.execute(script);

        CHECK(ctx.get_variable_value("x").int_value() == 10);
        CHECK(ctx.get_variable_value("y").int_value() == 8);
        CHECK(ctx.get_variable_value("z").int_value() == 8);
    }
}

TEST_CASE("script functions", "[script]") {
    constexpr std::size_t memsize = 128;
    ps::context ctx(memsize);

    SECTION("builtins") {
        std::string source = R"(
            __print(5 + 6);
        )";

        ps::script script(source, ctx);
        ctx.execute(script);
    }

    SECTION("simple functions") {
        std::string source = R"(
            fn print(x: int) -> int {
                // call builtin print function. eventually this version of print() will become library code (and call through std.io.print()).
                return __print(x);
            }

            fn sum(a: int, b: int) -> int {
                return a + b;
            }

            let x = sum(100, 200);
            print(x);
        )";

        ps::script script(source, ctx);
        ctx.execute(script);
    }
}