#include <pscript/context.hpp>
#include <algorithm>
#include <iostream>
#include <fstream>

#include <catch2/catch_test_macros.hpp>

template<typename T>
bool range_equal(ps::memory_pool const& memory, ps::pointer begin, ps::pointer end, T const& val) {
    for (ps::pointer it = begin; it < end; it += sizeof(T)) {
        T const& v = memory.get<T>(it);
        if (v != val) return false;
    }
    return true;
}

// requires that output stream is a std::ostringstream
static bool output_equal(ps::execution_context& exec, std::string const& expected) {
    return dynamic_cast<std::ostringstream*>(exec.out)->str() == expected;
}

float add(float a, float b) {
    return a + b;
}

void no_ret_type(int x) {
    std::cout << x << std::endl;
}

// external type must be integer
int print_int(ps::external const& ext) {
    std::cout << static_cast<int const&>(ext.value()) << std::endl;
    return 0;
}

struct external_struct {
    int x;
};

int print_ext_struct(ps::external const& ext) {
    std::cout << static_cast<external_struct const&>(ext.value()).x << std::endl;
    return 0;
}

void test_f([[maybe_unused]] int x) {

}

void lib_a() {
    std::cout << "lib a\n";
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
        CHECK(static_cast<int const&>(x.value()) == 5);

        // Variable shadowing
        ps::variable& x_float = ctx.create_variable("x", 3.14f);
        CHECK(static_cast<float const&>(x.value()) == 3.14f);
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
        CHECK(static_cast<int const&>(y) == 5);
        CHECK(static_cast<float const&>(z) == 4.4f);
    }

    SECTION("basic operations") {
        std::string source = R"(
            let x = 3 + 2;
            let y = x + 5;
            let z = x * x;
        )";

        ps::script script(source, ctx);
        ctx.execute(script);

        CHECK(static_cast<int const&>(ctx.get_variable_value("x")) == 5);
        CHECK(static_cast<int const&>(ctx.get_variable_value("y")) == 10);
        CHECK(static_cast<int const&>(ctx.get_variable_value("z")) == 25);
    }

    SECTION("parentheses and precedence") {
        std::string source = R"(
            let x = 2 * (3 + 2);
            let y = 2 * 3 + 2;
            let z = 2 + 2 * 3;
        )";

        ps::script script(source, ctx);
        ctx.execute(script);

        CHECK(static_cast<int const&>(ctx.get_variable_value("x")) == 10);
        CHECK(static_cast<int const&>(ctx.get_variable_value("y")) == 8);
        CHECK(static_cast<int const&>(ctx.get_variable_value("z")) == 8);
    }

    SECTION("floating point") {
        std::string source = R"(
            let x = 1.0 + 2.5;
            let y = 1.0 * 2.5;
            let z = 1 * 3.5;
            let w = 3.5 * 1;
        )";

        ps::script script(source, ctx);
        ctx.execute(script);

        CHECK(static_cast<float const&>(ctx.get_variable_value("x")) == 3.5f);
        CHECK(static_cast<float const&>(ctx.get_variable_value("y")) == 2.5f);
        CHECK(static_cast<float const&>(ctx.get_variable_value("z")) == 3.5f);
        CHECK(static_cast<float const&>(ctx.get_variable_value("w")) == 3.5f);
    }
}

TEST_CASE("floats in lists", "[script]") {
    constexpr std::size_t memsize = 512;
    ps::context ctx(memsize);

    SECTION("base case") {
        std::string source = R"(
            let l = [1.2];
            let a = l[0] * 0.5;
        )";

        ps::script script(source, ctx);
        ctx.execute(script);
        CHECK(static_cast<float const&>(ctx.get_variable_value("a")) == 0.6f);
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
        CHECK(static_cast<int const&>(ctx.get_variable_value("x")) == 1);
    }
}

TEST_CASE("control sequences", "[script]") {
    constexpr std::size_t memsize = 1024;
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

        CHECK(static_cast<int const&>(ctx.get_variable_value("f")) == 89);
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

        CHECK(static_cast<int const&>(ctx.get_variable_value("t")) == 15);
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
            __print(my_list);
        )";

        ps::script script(source, ctx);
        ctx.execute(script, exec);

        CHECK(output_equal(exec, "[1, 2, 3, 4, 5]\n"));
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

        CHECK(static_cast<int const&>(ctx.get_variable_value("x")) == 3);
        CHECK(static_cast<int const&>(ctx.get_variable_value("y")) == 4);
    }

    SECTION("memory") {
        std::string source = R"(
            import std.memory;

            std.memory.dump();
        )";

        ps::script script(source, ctx);
        ctx.execute(script);
    }
}

TEST_CASE("structs") {
    constexpr std::size_t memsize = 1024;
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

    SECTION("complex member access") {
        std::string source = R"(
            import std.io;

            struct MyStruct {
                a: int = 5;
                b: float = 6.6;
                c: list = [1, 2, 3];
            };

            let l = [ MyStruct { 1 }, MyStruct { 2 } ];
            std.io.print(l[1]->a);
            std.io.print(l[1]->c[2]); // should print 3
        )";

        ps::script script(source, ctx);
        ctx.execute(script);
    }
}

TEST_CASE("external functions") {
    constexpr size_t memsize = 1024;
    ps::context ctx(memsize);

    std::unique_ptr<ps::extern_library> lib = std::make_unique<ps::extern_library>();
    lib->add_function(ctx, "add", &add);
    lib->add_function(ctx, "print_extern", &no_ret_type);

    std::unique_ptr<ps::extern_library> other_lib = std::make_unique<ps::extern_library>();
    lib->add_function(ctx, "lib_a", &lib_a);

    auto extern_lib = ps::extern_library_chain_builder{}
        .add(std::move(lib))
        .add(std::move(other_lib))
        .get();

    ps::execution_context exec {};
    exec.externs = extern_lib.get();

    std::string source = R"(
        import std.io;

        extern fn add(a: float, b: float) -> float;
        extern fn print_extern(x: int) -> void;

        extern fn lib_a() -> void;

        std.io.print(add(1.1, 2.2));
        print_extern(1000);
        lib_a();
    )";

    ps::script script(source, ctx);
    ctx.execute(script, exec);
}

TEST_CASE("external types") {
    constexpr size_t memsize = 1024 * 1024;
    ps::context ctx(memsize);

    int my_integer = 10;
    external_struct my_struct { .x = 20 };

    ps::extern_library lib {};
    lib.add_variable("my_integer", &my_integer);
    lib.add_variable("my_struct", &my_struct);
    lib.add_function(ctx, "print_int", &print_int);
    lib.add_function(ctx, "print_ext_struct", &print_ext_struct);

    ps::execution_context exec {};
    exec.externs = &lib;

    SECTION("simple") {
        std::string source = R"(
            extern fn print_int(x: int) -> void;
            extern let my_integer -> int;

            print_int(my_integer);
        )";

        ps::script script(source, ctx);
        ctx.execute(script, exec);
    }

    // note that modifying external types is not properly supported yet (TODO?)
    SECTION("structs") {
        std::string source = R"(
            extern fn print_ext_struct(x: any) -> void;
            extern let my_struct -> any;

            __print(my_struct);
            print_ext_struct(my_struct);
        )";

        ps::script script(source, ctx);
        ctx.execute(script, exec);
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

TEST_CASE("error reporting") {
    constexpr size_t memsize = 1024 * 1024;
    ps::context ctx(memsize);

    ps::execution_context exec;

    SECTION("invalid syntax") {
        std::string source = R"(
            let x =;
        )";

        ps::script script(source, ctx);
        ctx.execute(script, exec);
    }

    SECTION("undeclared variable") {
        std::string source = R"(
            let x = y; // y is undeclared
        )";

        ps::script script(source, ctx);
        ctx.execute(script, exec);
    }

    SECTION("no extern lib - functions") {
        std::string source = R"(
            extern fn f() -> void;

            f();
        )";

        ps::script script(source, ctx);
        ctx.execute(script, exec);
    }

    SECTION("no extern lib - variables") {
        std::string source = R"(
            extern let x -> int;
            __print(x);
        )";

        ps::script script(source, ctx);
        ctx.execute(script, exec);
    }

    SECTION("function argument count") {
        std::string source = R"(
            fn foo(a: int, b: float) -> void {

            }

            foo(1, 2, 3);
        )";

        ps::script script(source, ctx);
        ctx.execute(script, exec);
    }

    SECTION("undefined function") {
        std::string source = R"(
            foo(1, 2, 3);
        )";

        ps::script script(source, ctx);
        ctx.execute(script, exec);
    }

    SECTION("external call with too many arguments") {
        auto externs = ps::extern_library {};
        externs.add_function(ctx, "foo", &test_f);
        exec.externs = &externs;

        std::string source = R"(
            extern fn foo(a1: int, a2: int, a3: int, a4: int, a5: int, a6: int, a7: int, a8: int, a9: int) -> int;

            foo(1, 2, 3, 4, 5, 6, 7, 8, 9);
        )";

        ps::script script(source, ctx);
        ctx.execute(script, exec);
    }

    SECTION("extern function not found") {
        auto externs = ps::extern_library {};
        exec.externs = &externs;

        std::string source = R"(
            extern fn foo() -> void;

            foo();
        )";

        ps::script script(source, ctx);
        ctx.execute(script, exec);
    }

    SECTION("extern variable not found") {
        auto externs = ps::extern_library {};
        exec.externs = &externs;

        std::string source = R"(
            extern let x -> int;

            let y = x;
        )";

        ps::script script(source, ctx);
        ctx.execute(script, exec);
    }

    SECTION("list append") {
        std::string source = R"(
            let l = [];
            l.append(1, 2);
        )";

        ps::script script(source, ctx);
        ctx.execute(script, exec);
    }

    SECTION("__print") {
        std::string source = R"(
            __print(1, 2);
        )";

        ps::script script(source, ctx);
        ctx.execute(script, exec);
    }

    SECTION("invalid import") {
        std::string source = R"(
            import xyz.meme_folder;
        )";

        ps::script script(source, ctx);
        ctx.execute(script, exec);
    }

    SECTION("invalid cast") {
        std::string source = R"(
            let s = str { 123 };
        )";

        ps::script script(source, ctx);
        ctx.execute(script, exec);
    }

    SECTION("undefined struct") {
        std::string source = R"(
            let x = Struct { 1, 2, "abc" };
        )";

        ps::script script(source, ctx);
        ctx.execute(script, exec);
    }
}

TEST_CASE("type checking") {
    constexpr size_t memsize = 1024;
    ps::context ctx(memsize);

    ps::execution_context exec;

    SECTION("return statements") {
        std::string source = R"(
            fn f() -> int {
                return "abc"; // TypeError
            }

            let x = f();
        )";

        ps::script script(source, ctx);
        ctx.execute(script, exec);
    }

    SECTION("variable assignment") {
        std::string source = R"(
            import std.io;
            import std.memory;

            let x = 4;
            x = 4.4;
            std.io.print(x);
            x = "abc"; // TypeError
        )";

        ps::script script(source, ctx);
        ctx.execute(script, exec);
    }

    SECTION("function parameters") {
        std::string source = R"(
            fn f(a: int, b: float) -> void {
                __print(a);
                __print(b);
            }

            f("abc", 4.7); // should throw a TypeError
        )";

        ps::script script(source, ctx);
        ctx.execute(script, exec);
    }

    SECTION("struct init") {
        std::string source = R"(
            struct S {
                x: int = "abc"; // should throw a TypeError
            };
        )";

        ps::script script(source, ctx);
        ctx.execute(script, exec);
    }

    SECTION("struct constructor") {
        std::string source = R"(
            struct S {
                x: int = 0;
            };

            let s = S { "abc" };
        )";

        ps::script script(source, ctx);
        ctx.execute(script, exec);
    }

    SECTION("different struct types") {
        std::string source = R"(
            struct S {
                x: int = 0;
            };

            struct T {
                y: str = "";
            };

            fn takes_s(x: S) -> void {

            }

            takes_s(T {});
        )";

        ps::script script(source, ctx);
        ctx.execute(script, exec);
    }

    SECTION("list stored type") {
        std::string source = R"(
            let l = [1, 2, 3];
            l.append(3.3); // TypeError
        )";

        ps::script script(source, ctx);
        ctx.execute(script, exec);
    }
}