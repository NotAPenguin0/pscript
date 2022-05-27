#include <catch2/catch_test_macros.hpp>

#include <pscript/context.hpp>
#include <algorithm>

template<typename T>
bool range_equal(ps::memory_pool const& memory, ps::pointer begin, ps::pointer end, T const& val) {
    for (ps::pointer it = begin; it < end; it += sizeof(T)) {
        T const& v = memory.get<T>(it);
        if (v != val) return false;
    }
    return true;
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

        REQUIRE(memory.verify_pointer(p));
        // Middle of address range
        REQUIRE(memory.verify_pointer(p + memsize / 2));
        // This is out of bounds
        REQUIRE(!memory.verify_pointer(p + memsize));
        // Null pointer can't be valid either
        REQUIRE(!memory.verify_pointer(ps::null_pointer));

        // Verify that memory is zeroed out on construction
        REQUIRE(range_equal(memory, memory.begin(), memory.end(), ps::byte{ 0x00 }));
    }

    SECTION("memory allocation") {
        ps::memory_pool& memory = ctx.memory();

        // Try to allocate some memory and verify pointers
        ps::pointer p0 = memory.allocate(10);
        ps::pointer p1 = memory.allocate(2);
        ps::pointer p2 = memory.allocate(1000);

        REQUIRE(memory.verify_pointer(p0));
        REQUIRE(memory.verify_pointer(p1));
        REQUIRE(memory.verify_pointer(p2));

        // Impossible to allocate
        ps::pointer p3 = memory.allocate(memsize + 1000);
        REQUIRE(p3 == ps::null_pointer);

        memory.free(p0);
        memory.free(p1);
        memory.free(p2);
    }

    SECTION("variables") {
        ps::memory_pool& memory = ctx.memory();

        ps::variable& x = ctx.create_variable("x", 5);
        REQUIRE(x.value().int_value(memory) == 5);

        // Variable shadowing
        ps::variable& x_float = ctx.create_variable("x", 3.14f);
        REQUIRE(x.value().fp_value(memory) == 3.14f);
    }
}