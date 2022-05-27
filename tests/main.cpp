#include <catch2/catch_test_macros.hpp>

TEST_CASE("check", "[abc]") {
    REQUIRE(1 == 1);
    REQUIRE(2 == 2);
}