#pragma once

#include <pscript/memory.hpp>

namespace ps {

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

private:
    ps::memory_pool mem;
};

}