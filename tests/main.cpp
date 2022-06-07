#include <pscript/context.hpp>
#include <algorithm>
#include <iostream>
#include <fstream>

#include <pscript/tokenizer.hpp>

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

// requires that output stream is a stringstream
static bool output_equal(ps::execution_context& exec, std::string const& expected) {
    return dynamic_cast<std::ostringstream*>(exec.out)->str() == expected;
}

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

TEST_CASE("script", "[script]") {
    constexpr std::size_t memsize = 128;
    ps::context ctx(memsize);

    std::ostringstream out {};
    ps::execution_context exec {};
    exec.out = &out;

    SECTION("builtins") {
        std::string source = R"(
            __print(5 + 6);
        )";

        ps::script script(source, ctx);
        ctx.execute(script, exec);

        CHECK(output_equal(exec, "11\n"));
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
        ctx.execute(script, exec);

        CHECK(output_equal(exec, "300\n"));
    }

    SECTION("local scopes") {
        std::string source = R"(
            fn foo() -> void {
                let x = 5;
                x = 10;
            }

            let x = 1;
            foo();
        )";

        ps::script script(source, ctx);
        ctx.execute(script);
        // after executing foo(), the  original value in 'x' should be unmodified.
        CHECK(ctx.get_variable_value("x").int_value() == 1);
    }
}

TEST_CASE("control sequences", "[script]") {
    constexpr std::size_t memsize = 512;
    ps::context ctx(memsize);

    std::ostringstream out {};
    ps::execution_context exec {};
    exec.out = &out;

    SECTION("if") {
        std::string source = R"(
            fn print(x: int) -> int {
                // call builtin print function. eventually this version of print() will become library code (and call through std.io.print()).
                return __print(x);
            }

            let x = 5;
            let y = 10;
            if (x < y) {
                // should be able to look up x from parent scope
                print(x);
            } else {
                print(y);
            }
        )";

        ps::script script(source, ctx);
        ctx.execute(script, exec);

        CHECK(output_equal(exec, "5\n"));
    }

    SECTION("recursion") {
        std::string source = R"(
            fn fib(n: int) -> int {
                if (n == 0) return 0;
                else if (n == 1) return 1;
                else return fib(n - 1) + fib(n - 2);
            }

            let f = fib(11);
        )";

        ps::script script(source, ctx);
        ctx.execute(script);

        CHECK(ctx.get_variable_value("f").int_value() == 89);
    }

    SECTION("while") {
        std::string source = R"(
            fn triangle(n: int) -> int {
                let a = 0;
                while(n > 0) {
                    a += n;
                    n -= 1;
                }
                return a;
            }

            let t = triangle(5);
        )";

        ps::script script(source, ctx);
        ctx.execute(script);

        CHECK(ctx.get_variable_value("t").int_value() == 15);
    }
}

TEST_CASE("lists") {
    constexpr std::size_t memsize = 512;
    ps::context ctx(memsize);

    std::ostringstream out {};
    ps::execution_context exec {};
    exec.out = &out;

    SECTION("append") {
        std::string source = R"(
            let my_list = [1, 2, 3];
            my_list.append(4);
            my_list.append(5);
            my_list.append(6.6);
            __print(my_list);
        )";

        ps::script script(source, ctx);
        ctx.execute(script, exec);

        CHECK(output_equal(exec, "[1, 2, 3, 4, 5, 6.6]\n"));
    }

    SECTION("indexing") {
        std::string source = R"(
            let my_list = [1, 2, 3];
            my_list[1] = 10;
            __print(my_list[1]);
        )";

        ps::script script(source, ctx);
        ctx.execute(script, exec);

        CHECK(output_equal(exec, "10\n"));
    }

    SECTION("size query") {
        std::string source = R"(
            let my_list = [1, 2, 3];
            __print(my_list.size());
        )";

        ps::script script(source, ctx);
        ctx.execute(script, exec);
        CHECK(output_equal(exec, "3\n"));
    }
}

TEST_CASE("reference types") {
    constexpr std::size_t memsize = 512;
    ps::context ctx(memsize);

    std::ostringstream out {};
    ps::execution_context exec {};
    exec.out = &out;

    std::string source = R"(
        import std.io;
        fn f(x: list) -> void {
            x[1] = 3;
        }

        let l = [1, 1, 1];
        f(l);
        // expected result: 3
        std.io.print(l[1]);
    )";

    ps::script script(source, ctx);
    ctx.execute(script, exec);
    CHECK(output_equal(exec, "3\n"));
}

TEST_CASE("strings") {
    constexpr std::size_t memsize = 4096;
    ps::context ctx(memsize);

    std::ostringstream out {};
    std::istringstream in {"10\n20\n"};
    ps::execution_context exec {};
    exec.out = &out;
    exec.in = &in;

    SECTION("basics") {
        std::string source = R"(
            import std.io;

            let my_str = "foobar";
            std.io.print(my_str);
        )";

        ps::script script(source, ctx);
        ctx.execute(script, exec);

        CHECK(output_equal(exec, "foobar\n"));
    }

    SECTION("concatenation") {
        std::string source = R"(
            import std.io;

            let x = "ABC";
            let y = "DEF";
            std.io.print(x + y);
        )";

        ps::script script(source, ctx);
        ctx.execute(script, exec);

        CHECK(output_equal(exec, "ABCDEF\n"));
    }

    SECTION("formatting") {
        std::string source = R"(
            import std.io;
            import std.string;
            std.io.print(std.string.format("Hello, {}", ["pengu"]));

            let my_list = [1, 2, 3];
            let fmt = "list = {}";
            std.io.print(fmt.format(my_list));
        )";

        ps::script script(source, ctx);
        ctx.execute(script, exec);

        CHECK(output_equal(exec, "Hello, pengu\nlist = [1, 2, 3]\n"));
    }


    SECTION("input") {
        std::string source = R"(
            import std.io;

            std.io.print("Enter first value: ");
            let x = std.io.read_int();
            std.io.print("Enter second value: ");
            let y = std.io.read_int();
            let fmt = "{} + {} = {}";
            std.io.print(fmt.format(x, y, x + y));
        )";

        ps::script script(source, ctx);
        ctx.execute(script, exec);

        CHECK(output_equal(exec, "Enter first value: \nEnter second value: \n10 + 20 = 30\n"));
    }
}

TEST_CASE("modules") {
    constexpr std::size_t memsize = 512;
    ps::context ctx(memsize);

    std::ostringstream out {};
    ps::execution_context exec {};
    exec.out = &out;

    SECTION("std import") {
        std::string source = R"(
            import std.io;
            std.io.print(5);
        )";

        ps::script script(source, ctx);
        ctx.execute(script, exec);

        CHECK(output_equal(exec, "5\n"));
    }
}

TEST_CASE("stdlib") {
    constexpr std::size_t memsize = 512;
    ps::context ctx(memsize);

    SECTION("math") {
        std::string source = R"(
            import std.math;
            import std.io;

            let x = std.math.min(3, 4);
            let y = std.math.max(3, 4);
        )";

        ps::script script(source, ctx);
        ctx.execute(script);

        CHECK(ctx.get_variable_value("x").int_value().value() == 3);
        CHECK(ctx.get_variable_value("y").int_value().value() == 4);
    }
}

TEST_CASE("structs") {
    constexpr std::size_t memsize = 512;
    ps::context ctx(memsize);

    SECTION("declaration") {
        std::string source = R"(
            struct MyStruct {
                a: int = 5;
                b: float = 6.6;
            };
        )";

        ps::script script(source, ctx);
        ctx.execute(script);
    }

    SECTION("construction") {
        std::string source = R"(
            import std.io;

            struct MyStruct {
                a: int = 5;
                b: float = 6.6;
            };

            let m = MyStruct { 1 };
            // std.io.print(m);
        )";

        ps::script script(source, ctx);
        ctx.execute(script);
    }

    SECTION("member access") {
        std::string source = R"(
            import std.io;

            struct MyStruct {
                a: int = 5;
                b: float = 6.6;
            };

            let m = MyStruct { 1 };
            // std.io.print(m->a);
            m->b = 4.0;
            // std.io.print(m->b);
        )";

        ps::script script(source, ctx);
        ctx.execute(script);
    }
}

TEST_CASE("perceptron") {
    constexpr size_t memsize = 1024 * 1024;
    ps::context ctx(memsize);

    std::ifstream infile {"samples/perceptron.ps"};
    std::string source = std::string{std::istreambuf_iterator<char>(infile), {}};

    ps::script script(source, ctx);
    ctx.execute(script);
}