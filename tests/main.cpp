#include <pscript/context.hpp>
#include <algorithm>
#include <iostream>

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
        CHECK(x.value().int_value(memory) == 5);

        // Variable shadowing
        ps::variable& x_float = ctx.create_variable("x", 3.14f);
        CHECK(x.value().fp_value(memory) == 3.14f);
    }
}

TEST_CASE("script creation", "[script]") {
    constexpr std::size_t memsize = 1024 * 1024;
    ps::context ctx(memsize);

    SECTION("tokenizer") {
        std::string src = "let x = 5;\nlet y = 9.7;";
        ps::script script(std::move(src));

        auto const& tokens = script.tokens();
        CHECK_THAT(tokens, TokenEqualMatcher(
            {"let", "x", "=", "5", ";", "let", "y", "=", "9.7", ";"}
        ));

        std::string src_with_function = "let z = function_call(a, 3.7, c, \"xyz\");";
        ps::script script_funcs(std::move(src_with_function));
        CHECK_THAT(script_funcs.tokens(), TokenEqualMatcher(
        {"let", "z", "=", "function_call", "(", "a", ",", "3.7", ",", "c", ",", "\"xyz\"", ")", ";"}
        ));
    }

    SECTION("lexer") {
        std::string src = "let z = function_call(a + 3, 3.7, c, \"xyz\");";
        ps::script script(std::move(src));
        CHECK_THAT(script.tokens(), TokenTypeMatcher(
            {ps::token_type::keyword, ps::token_type::identifier, ps::token_type::op, ps::token_type::identifier, ps::token_type::parenthesis,
             ps::token_type::identifier, ps::token_type::op, ps::token_type::constant, ps::token_type::comma, ps::token_type::constant, ps::token_type::comma,
             ps::token_type::identifier, ps::token_type::comma, ps::token_type::constant, ps::token_type::parenthesis, ps::token_type::semicolon }
        ));
    }
}