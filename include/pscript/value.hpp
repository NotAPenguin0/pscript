#pragma once

#include <pscript/memory.hpp>

namespace ps {

namespace types {

using integer = int;
using real = float;

}

/**
 * @brief Represents a typed value. This can be used in the context of a variable (with a name), or as a constant (anonymous).
 */
class value {
public:
    enum class type {
        null,
        integer,
        // float
        real,
        str,
        list,
        // external functions or classes.
        external_object
    };

    value(value const&);
    value& operator=(value const&);

    value(value&& rhs) noexcept;
    value& operator=(value&& rhs) noexcept;

    ~value();

    // Creates a new value with matching type, allocates memory for it and initializes it.

    static ps::value null();
    static ps::value from(ps::memory_pool& memory, int v);
    static ps::value from(ps::memory_pool& memory, float v);

    ps::pointer pointer();
    type get_type() const;

    ps::types::integer& int_value();
    ps::types::real& real_value();

    ps::types::integer const& int_value() const;
    ps::types::real const& real_value() const;

    ps::value operator+(ps::value const& rhs) const;
    ps::value operator*(ps::value const& rhs) const;

private:
    value() = default;

    ps::memory_pool* memory = nullptr;

    // Pointer to allocated memory for this value.
    ps::pointer ptr = ps::null_pointer;
    type tpe{};
};

}