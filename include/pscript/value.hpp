#pragma once

#include <pscript/memory.hpp>

namespace ps {

namespace types {

using integer = int;
using real = float;
using boolean = bool;

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
        boolean,
        str,
        list,
        // external functions or classes.
        external_object
    };

    value(value const& rhs);
    value& operator=(value const& rhs);

    value(value&& rhs) noexcept;
    value& operator=(value&& rhs) noexcept;

    ~value();

    // Creates a new value with matching type, allocates memory for it and initializes it.

    static ps::value null();
    static ps::value from(ps::memory_pool& memory, int v);
    static ps::value from(ps::memory_pool& memory, float v);
    static ps::value from(ps::memory_pool& memory, bool v);

    ps::pointer pointer();
    type get_type() const;

    inline bool is_null() const { return tpe == type::null; }

    ps::types::integer& int_value();
    ps::types::real& real_value();

    ps::types::integer const& int_value() const;
    ps::types::real const& real_value() const;

    template<typename T>
    explicit operator T&() {
        return memory->get<T>(ptr);
    }

    template<typename T>
    explicit operator T const& () const {
        return memory->get<T>(ptr);
    }

    inline ps::memory_pool& get_memory() const {
        return *memory;
    }

private:
    value() = default;

    mutable ps::memory_pool* memory = nullptr;

    // Pointer to allocated memory for this value.
    ps::pointer ptr = ps::null_pointer;
    type tpe{};
};

template<typename F>
void visit_value(value& v, F&& callable) {
    switch(v.get_type()) {
        case value::type::null:
            break;
        case value::type::integer:
            callable(static_cast<types::integer&>(v));
            break;
        case value::type::real:
            callable(static_cast<types::real&>(v));
            break;
        case value::type::boolean:
            callable(static_cast<types::boolean&>(v));
            break;
        case value::type::str:
            break;
        case value::type::list:
            break;
        case value::type::external_object:
            break;
    }
}

template<typename F>
void visit_value(value const& v, F&& callable) {
    switch(v.get_type()) {
        case value::type::null:
            break;
        case value::type::integer:
            callable(static_cast<types::integer const&>(v));
            break;
        case value::type::real:
            callable(static_cast<types::real const&>(v));
            break;
        case value::type::boolean:
            callable(static_cast<types::boolean const&>(v));
            break;
        case value::type::str:
            break;
        case value::type::list:
            break;
        case value::type::external_object:
            break;
    }
}

#define GEN_VALUE_OP(op) inline ps::value operator op (ps::value const& lhs, ps::value const& rhs) { \
    value result = value::null();                                                                    \
    visit_value(lhs, [&result, &lhs, &rhs] (auto const& lhs_val) {                                   \
        visit_value(rhs, [&result, &lhs, &lhs_val] (auto const& rhs_val) {                           \
            result = value::from(lhs.get_memory(), lhs_val op rhs_val);                              \
        });                                                                                          \
    });                                                                                              \
    return result;                                                                                   \
}

#define GEN_MUTABLE_OP(op) inline ps::value& operator op##= (ps::value& lhs, ps::value const& rhs) { \
    visit_value(lhs, [&lhs, &rhs](auto const& lhs_val) {                                          \
        visit_value(rhs, [&lhs, &lhs_val] (auto const& rhs_val) {                                 \
           lhs = value::from(lhs.get_memory(), lhs_val op rhs_val);                               \
        });                                                                                       \
    });                                                                                           \
    return lhs;                                                                                   \
}

GEN_VALUE_OP(+)
GEN_VALUE_OP(-)
GEN_VALUE_OP(*)
GEN_VALUE_OP(/)
GEN_VALUE_OP(==)
GEN_VALUE_OP(!=)
GEN_VALUE_OP(<)
GEN_VALUE_OP(>)
GEN_VALUE_OP(<=)
GEN_VALUE_OP(>=)

// +=, -=, *=, /=
GEN_MUTABLE_OP(+)
GEN_MUTABLE_OP(-);
GEN_MUTABLE_OP(*);
GEN_MUTABLE_OP(/);

#undef GEN_VALUE_OP
#undef GEN_MUTABLE_OP

}