#pragma once

#include <cstddef>

namespace plib {

// returns the next power of two
inline std::size_t next_pow_two(std::size_t v) {
    // Source: https://stackoverflow.com/questions/466204/rounding-up-to-next-power-of-2
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    if constexpr (sizeof(std::size_t) == 8) {
        v |= v >> 32;
    }
    return v + 1;
}

} // namespace plib