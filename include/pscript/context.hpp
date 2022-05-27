#pragma once

#include <pscript/memory.hpp>
#include <pscript/variable.hpp>

#include <string>
#include <unordered_map>

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

    /**
     * @brief Creates a new variable with an initializer.
     * @throws std::runtime_error on failure.
     * @tparam T Must be a valid initializer type (a type that can be converted to a pscript type.
     * @param name Name of the variable. If this is the name of a previously created variable it will be overwritten
     * @param initializer Value to initialize the variable with.
     * @return Reference to the created variable.
     */
    template<typename T>
    [[nodiscard]] ps::variable& create_variable(std::string const& name, T const& initializer) {
        if (auto old = variables.find(name); old != variables.end()) {
            // Variable already exists, so shadow it with a new type by assigning a new value to it.
            // We first need to free the old memory
            memory().free(old->second.value().pointer());
            old->second.value() = ps::value::from(memory(), initializer);
            return old->second;
        } else {
            auto it = variables.insert({name, variable(memory(), name, initializer)});
            // make sure name string view points to the entry in the map, so it is guaranteed to live as long as the variable lives.
            it.first->second.set_name(it.first->first);
            return it.first->second;
        }
    }

private:
    ps::memory_pool mem;
    std::unordered_map<std::string, ps::variable> variables;
};

}