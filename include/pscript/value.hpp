#pragma once

#include <pscript/memory.hpp>

namespace ps {

namespace types {

using integer = int;
using fp = float;

}

/**
 * @brief Represents a typed value. This can be used in the context of a variable (with a name), or as a constant (anonymous).
 */
class value {
public:
    enum class type {
        integer,
        // float
        fp,
        str,
        list,
        // external functions or classes.
        external_object
    };

    // Creates a new value with matching type, allocates memory for it and initializes it.

    static ps::value from(ps::memory_pool& memory, int v);
    static ps::value from(ps::memory_pool& memory, float v);

    ps::pointer pointer();
    type get_type();

    ps::types::integer& int_value(ps::memory_pool& memory);
    ps::types::fp& fp_value(ps::memory_pool& memory);

    ps::types::integer const& int_value(ps::memory_pool const& memory) const;
    ps::types::fp const& fp_value(ps::memory_pool const& memory) const;

private:
    value() = default;

    // Pointer to allocated memory for this value.
    ps::pointer ptr = ps::null_pointer;
    type tpe{};
};

}