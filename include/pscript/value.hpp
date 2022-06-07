#pragma once

#include <pscript/memory.hpp>

#include <plib/concepts.hpp>

#include <vector>
#include <exception>
#include <stdexcept>
#include <iostream>

namespace ps {

class value;

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

template<typename T>
class value_storage {
public:
    using value_type = T;

    value_storage() = default;

    explicit value_storage(T const& v) : val(v) {

    }

    value_storage(value_storage const& rhs) : val(rhs.value) {

    }

    value_storage(value_storage&& rhs)  noexcept : val(std::move(rhs.val)) {

    }

    value_storage& operator=(T const& rhs) {
        val = rhs;
        return *this;
    }

    value_storage& operator=(value_storage const& rhs) {
        val = rhs.val;
        return *this;
    }

    value_storage& operator=(value_storage&& rhs) noexcept {
        val = std::move(rhs.val);
        return *this;
    }

    explicit operator T& () {
        return val;
    }

    explicit operator T const& () const {
        return val;
    }

    T& value() {
        return val;
    }

    T const& value() const {
        return val;
    }

    T* operator->() {
        return &val;
    }

    T const* operator->() const {
        return &val;
    }

protected:
    T val = {};
};

template<typename T>
class eq_comparable : public value_storage<T> {
public:
    using value_storage<T>::value_storage;
    using value_storage<T>::operator=;
};

template<typename T>
class rel_comparable : public eq_comparable<T> {
public:
    using eq_comparable<T>::eq_comparable;
    using eq_comparable<T>::operator=;
};

template<typename T>
class arithmetic_type : public rel_comparable<T> {
public:
    using rel_comparable<T>::rel_comparable;
    using rel_comparable<T>::operator=;
};

// Fallbacks

template<typename T, typename U>
bool operator==(value_storage<T> const& lhs, value_storage<U> const& rhs) {
    throw std::runtime_error("operator== not supported for this type");
}

template<typename T, typename U>
bool operator!=(value_storage<T> const& lhs, value_storage<U> const& rhs) {
    throw std::runtime_error("operator!= not supported for this type");
}

template<typename T, typename U>
bool operator<(value_storage<T> const& lhs, value_storage<U> const& rhs) {
    throw std::runtime_error("operator< not supported for this type");
}

template<typename T, typename U>
bool operator<=(value_storage<T> const& lhs, value_storage<U> const& rhs) {
    throw std::runtime_error("operator<= not supported for this type");
}

template<typename T, typename U>
bool operator>(value_storage<T> const& lhs, value_storage<U> const& rhs) {
    throw std::runtime_error("operator> not supported for this type");
}

template<typename T, typename U>
bool operator>=(value_storage<T> const& lhs, value_storage<U> const& rhs) {
    throw std::runtime_error("operator>= not supported for this type");
}

template<typename T, typename U>
T operator+(value_storage<T> const& lhs, value_storage<U> const& rhs) {
    throw std::runtime_error("operator+ not supported for this type");
}

template<typename T, typename U>
T operator-(value_storage<T> const& lhs, value_storage<U> const& rhs) {
    throw std::runtime_error("operator- not supported for this type");
}

template<typename T, typename U>
T operator*(value_storage<T> const& lhs, value_storage<U> const& rhs) {
    throw std::runtime_error("operator* not supported for this type");
}

template<typename T, typename U>
T operator/(value_storage<T> const& lhs, value_storage<U> const& rhs) {
    throw std::runtime_error("operator/ not supported for this type");
}

template<typename T>
std::ostream& operator<<(std::ostream& out, value_storage<T> const& value) {
    return out << value.value();
}

// Actual implementations for types that do support the operations.

template<typename T, typename U> requires std::equality_comparable_with<T, U>
bool operator==(eq_comparable<T> const& lhs, eq_comparable<U> const& rhs) {
    return lhs.value() == rhs.value();
}

template<typename T, typename U> requires std::is_pod_v<U> && std::equality_comparable_with<T, U>
bool operator==(eq_comparable<T> const& lhs, U const& rhs) {
    return lhs.value() == rhs;
}

template<typename T, typename U> requires std::equality_comparable_with<T, U>
bool operator!=(eq_comparable<T> const& lhs, eq_comparable<U> const& rhs) {
    return lhs.value() != rhs.value();
}

template<typename T, typename U> requires std::is_pod_v<U> && std::equality_comparable_with<T, U>
bool operator!=(eq_comparable<T> const& lhs, U const& rhs) {
    return lhs.value() != rhs;
}

template<typename T, typename U> requires std::totally_ordered_with<T, U>
bool operator<(rel_comparable<T> const& lhs, rel_comparable<U> const& rhs) {
    return lhs.value() < rhs.value();
}

template<typename T, typename U> requires std::is_pod_v<U> && std::totally_ordered_with<T, U>
bool operator<(rel_comparable<T> const& lhs, U const& rhs) {
    return lhs.value() < rhs;
}

template<typename T, typename U> requires std::totally_ordered_with<T, U>
bool operator<=(rel_comparable<T> const& lhs, rel_comparable<U> const& rhs) {
    return lhs.value() <= rhs.value();
}

template<typename T, typename U> requires std::is_pod_v<U> && std::totally_ordered_with<T, U>
bool operator<=(rel_comparable<T> const& lhs, U const& rhs) {
    return lhs.value() <= rhs;
}

template<typename T, typename U> requires std::totally_ordered_with<T, U>
bool operator>(rel_comparable<T> const& lhs, rel_comparable<U> const& rhs) {
    return lhs.value() > rhs.value();
}

template<typename T, typename U> requires std::is_pod_v<U> && std::totally_ordered_with<T, U>
bool operator>(rel_comparable<T> const& lhs, U const& rhs) {
    return lhs.value() > rhs;
}

template<typename T, typename U> requires std::totally_ordered_with<T, U>
bool operator>=(rel_comparable<T> const& lhs, rel_comparable<U> const& rhs) {
    return lhs.value() >= rhs.value();
}

template<typename T, typename U> requires std::is_pod_v<U> && std::totally_ordered_with<T, U>
bool operator>=(rel_comparable<T> const& lhs, U const& rhs) {
    return lhs.value() >= rhs;
}

template<typename T, typename U> requires plib::adds<T, U>
std::common_type_t<T, U> operator+(arithmetic_type<T> const& lhs, arithmetic_type<U> const& rhs) {
    return lhs.value() + rhs.value();
}

template<typename T, typename U> requires std::is_pod_v<U> && plib::adds<T, U>
std::common_type_t<T, U> operator+(arithmetic_type<T> const& lhs, U const& rhs) {
    return lhs.value() + rhs;
}

template<typename T, typename U> requires plib::subtracts<T, U>
std::common_type_t<T, U> operator-(arithmetic_type<T> const& lhs, arithmetic_type<U> const& rhs) {
    return lhs.value() - rhs.value();
}

template<typename T, typename U> requires std::is_pod_v<U> && plib::subtracts<T, U>
std::common_type_t<T, U> operator-(arithmetic_type<T> const& lhs, U const& rhs) {
    return lhs.value() - rhs;
}

template<typename T, typename U> requires plib::multiplies<T, U>
std::common_type_t<T, U> operator*(arithmetic_type<T> const& lhs, arithmetic_type<U> const& rhs) {
    return lhs.value() * rhs.value();
}

template<typename T, typename U> requires std::is_pod_v<U> && plib::multiplies<T, U>
std::common_type_t<T, U> operator*(arithmetic_type<T> const& lhs, U const& rhs) {
    return lhs.value() * rhs;
}

template<typename T, typename U> requires plib::divides<T, U>
std::common_type_t<T, U> operator/(arithmetic_type<T> const& lhs, arithmetic_type<U> const& rhs) {
    return lhs.value() / rhs.value();
}

template<typename T, typename U> requires std::is_pod_v<U> && plib::divides<T, U>
std::common_type_t<T, U> operator/(arithmetic_type<T> const& lhs, U const& rhs) {
    return lhs.value() / rhs;
}

class list_type {
public:
    list_type() = default;
    explicit list_type(std::vector<ps::value> const& values);
    list_type(list_type const&) = default;
    list_type(list_type&&) noexcept = default;
    list_type& operator=(list_type const&) = default;
    list_type& operator=(list_type&&) noexcept = default;

    void append(ps::value const& val);

    ps::value& get(size_t index);
    size_t size() const;

    friend std::ostream& operator<<(std::ostream& out, list_type const& list);

private:
    std::vector<ps::value> storage;
    type stored_type {};
};

using integer = arithmetic_type<int>;
using real = arithmetic_type<float>;
using boolean = eq_comparable<bool>;
using list = value_storage<list_type>;

/**
 * @brief Represents a typed value. This can be used in the context of a variable (with a name), or as a constant (anonymous).
 */
class value {
public:
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
    static ps::value from(ps::memory_pool& memory, ps::list_type const& v);

    ps::pointer pointer();
    type get_type() const;

    inline bool is_null() const { return tpe == type::null; }

    ps::integer& int_value();
    ps::real& real_value();

    ps::integer const& int_value() const;
    ps::real const& real_value() const;

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
        case type::null:
            break;
        case type::integer:
            callable(static_cast<ps::integer&>(v));
            break;
        case type::real:
            callable(static_cast<ps::real&>(v));
            break;
        case type::boolean:
            callable(static_cast<ps::boolean&>(v));
            break;
        case type::str:
            break;
        case type::list:
            callable(static_cast<ps::list&>(v));
            break;
        case type::external_object:
            break;
        default:
            break;
    }
}

template<typename F>
void visit_value(value const& v, F&& callable) {
    switch(v.get_type()) {
        case type::null:
            break;
        case type::integer:
            callable(static_cast<ps::integer const&>(v));
            break;
        case type::real:
            callable(static_cast<ps::real const&>(v));
            break;
        case type::boolean:
            callable(static_cast<ps::boolean const&>(v));
            break;
        case type::str:
            break;
        case type::list:
            callable(static_cast<ps::list const&>(v));
            break;
        case type::external_object:
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

namespace std {

template<typename T, typename U>
struct common_type<ps::value_storage<T>, U> {
    using type = typename common_type<T, U>::type;
};

template<typename T, typename U>
struct common_type<ps::eq_comparable<T>, U> {
    using type = typename common_type<T, U>::type;
};

template<typename T, typename U>
struct common_type<ps::rel_comparable<T>, U> {
    using type = typename common_type<T, U>::type;
};

template<typename T, typename U>
struct common_type<ps::arithmetic_type<T>, U> {
    using type = typename common_type<T, U>::type;
};

template<typename T>
struct common_type<T, ps::list_type> {
    using type = void;
};

}