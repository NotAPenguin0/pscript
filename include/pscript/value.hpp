#pragma once

#include <pscript/memory.hpp>

#include <plib/concepts.hpp>

#include <vector>
#include <exception>
#include <stdexcept>
#include <iostream>
#include <unordered_map>

namespace ps {

class value;

enum class type {
    null,
    any,
    integer,
    uint,
    // float
    real,
    boolean,
    str,
    list,
    structure,
    external // stores an additional type member for its contained type.
};

bool may_cast(type from, type to);

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

template<typename T>
bool operator!(value_storage<T> const& lhs) {
    throw std::runtime_error("operator! not supported for this type");
}

template<typename T, typename U>
T operator&&(value_storage<T> const& lhs, value_storage<U> const& rhs) {
    throw std::runtime_error("operator&& not supported for this type");
}

template<typename T, typename U>
T operator||(value_storage<T> const& lhs, value_storage<U> const& rhs) {
    throw std::runtime_error("operator|| not supported for this type");
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

template<typename T>
T operator-(value_storage<T> const& lhs) {
    throw std::runtime_error("unary operator- not supported for this type");
}

template<typename T, typename U>
T operator*(value_storage<T> const& lhs, value_storage<U> const& rhs) {
    throw std::runtime_error("operator* not supported for this type");
}

template<typename T, typename U>
T operator/(value_storage<T> const& lhs, value_storage<U> const& rhs) {
    throw std::runtime_error("operator/ not supported for this type");
}

template<typename T, typename U>
T operator<<(value_storage<T> const& lhs, value_storage<U> const& rhs) {
    throw std::runtime_error("operator<< not supported for this type");
}

template<typename T, typename U>
T operator>>(value_storage<T> const& lhs, value_storage<U> const& rhs) {
    throw std::runtime_error("operator>> not supported for this type");
}

template<typename T, typename U>
T operator^(value_storage<T> const& lhs, value_storage<U> const& rhs) {
    throw std::runtime_error("operator^ not supported for this type");
}

template<typename T, typename U>
T operator&(value_storage<T> const& lhs, value_storage<U> const& rhs) {
    throw std::runtime_error("operator& not supported for this type");
}

template<typename T, typename U>
T operator%(value_storage<T> const& lhs, value_storage<U> const& rhs) {
    throw std::runtime_error("operator% not supported for this type");
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

template<typename T>
T operator-(arithmetic_type<T> const& lhs) {
    // unary operator-
    return -lhs.value();
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

template<typename T, typename U> requires plib::lshift<T, U>
std::common_type_t<T, U> operator<<(arithmetic_type<T> const& lhs, arithmetic_type<U> const& rhs) {
    return lhs.value() << rhs.value();
}

template<typename T, typename U> requires std::is_pod_v<U> && plib::lshift<T, U>
std::common_type_t<T, U> operator<<(arithmetic_type<T> const& lhs, U const& rhs) {
    return lhs.value() << rhs;
}

template<typename T, typename U> requires plib::rshift<T, U>
std::common_type_t<T, U> operator>>(arithmetic_type<T> const& lhs, arithmetic_type<U> const& rhs) {
    return lhs.value() >> rhs.value();
}

template<typename T, typename U> requires std::is_pod_v<U> && plib::rshift<T, U>
std::common_type_t<T, U> operator>>(arithmetic_type<T> const& lhs, U const& rhs) {
    return lhs.value() >> rhs;
}

template<typename T, typename U> requires plib::xors<T, U>
std::common_type_t<T, U> operator^(arithmetic_type<T> const& lhs, arithmetic_type<U> const& rhs) {
    return lhs.value() ^ rhs.value();
}

template<typename T, typename U> requires std::is_pod_v<U> && plib::xors<T, U>
std::common_type_t<T, U> operator^(arithmetic_type<T> const& lhs, U const& rhs) {
    return lhs.value() ^ rhs;
}

template<typename T, typename U> requires plib::bitwise_and<T, U>
std::common_type_t<T, U> operator&(arithmetic_type<T> const& lhs, arithmetic_type<U> const& rhs) {
    return lhs.value() & rhs.value();
}

template<typename T, typename U> requires std::is_pod_v<U> && plib::bitwise_and<T, U>
std::common_type_t<T, U> operator&(arithmetic_type<T> const& lhs, U const& rhs) {
    return lhs.value() & rhs;
}


template<typename T, typename U> requires plib::bitwise_and<T, U>
std::common_type_t<T, U> operator%(arithmetic_type<T> const& lhs, arithmetic_type<U> const& rhs) {
    return lhs.value() % rhs.value();
}

template<typename T, typename U> requires std::is_pod_v<U> && plib::modulo<T, U>
std::common_type_t<T, U> operator%(arithmetic_type<T> const& lhs, U const& rhs) {
    return lhs.value() % rhs;
}

// TODO: replace stored vector and string by custom types that use our memory allocator

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

    std::string to_string() const;

    friend std::ostream& operator<<(std::ostream& out, list_type const& list);

    inline std::vector<ps::value> const& representation() const { return storage; }

    template<typename T>
    explicit operator T() const {
        throw std::runtime_error("Invalid cast");
    }

private:
    std::vector<ps::value> storage;
    type stored_type {};
};

class string_type {
public:
    string_type() = default;

    explicit string_type(std::string const& str);
    string_type(string_type const&) = default;
    string_type(string_type&&) noexcept = default;
    string_type& operator=(string_type const&) = default;
    string_type& operator=(string_type&&) noexcept = default;

    [[nodiscard]] string_type format(std::vector<ps::value> const& args) const;

    int parse_int() const;
    float parse_float() const;

    friend std::ostream& operator<<(std::ostream& out, string_type const& str);

    inline std::string const& representation() const { return storage; }

    template<typename T>
    explicit operator T() const {
        throw std::runtime_error("Invalid cast");
    }

private:
    std::string storage {};
};

class struct_type {
public:
    struct_type() = default;
    explicit struct_type(std::unordered_map<std::string, ps::value> const& initializers);
    struct_type(struct_type const&) = default;
    struct_type(struct_type&&) noexcept = default;
    struct_type& operator=(struct_type const&) = default;
    struct_type& operator=(struct_type&&) noexcept = default;

    std::string to_string() const;

    ps::value& access(std::string const& name);
    ps::value const& access(std::string const& name) const;

    friend std::ostream& operator<<(std::ostream& out, string_type const& str);

    template<typename T>
    explicit operator T() const {
        throw std::runtime_error("Invalid cast");
    }

private:
    std::unordered_map<std::string, ps::value> members;
};

struct external_type {
public:
    external_type() = default;
    external_type(void* pointer, ps::type type);
    external_type(external_type const&) = default;
    external_type(external_type&&) noexcept = default;
    external_type& operator=(external_type const&) = default;
    external_type& operator=(external_type&&) noexcept = default;

    friend std::ostream& operator<<(std::ostream& out, external_type const& str);

    template<typename T>
    explicit operator T&()  {
        return *reinterpret_cast<T*>(ptr);
    }

    template<typename T>
    explicit operator T const&() const {
        return *reinterpret_cast<T const*>(ptr);
    }

    inline ps::type stored_type() const {
        return type;
    }

    inline void* pointer() const {
        return ptr;
    }

private:
    void* ptr;
    ps::type type;
};

using integer = arithmetic_type<int>;
using uint = arithmetic_type<unsigned int>;
using real = arithmetic_type<float>;
using boolean = eq_comparable<bool>;
using list = value_storage<list_type>;
using str = eq_comparable<string_type>;
using structure = value_storage<struct_type>;
using external = value_storage<external_type>;

inline bool operator&&(boolean const& lhs, boolean const &rhs) {
    return lhs.value() && rhs.value();
}

inline bool operator||(boolean const& lhs, boolean const &rhs) {
    return lhs.value() || rhs.value();
}

// string concatenation
inline string_type operator+(str const& lhs, str const& rhs) {
    return string_type { lhs->representation() + rhs->representation() };
}

inline bool operator!(boolean const& lhs) {
    return !lhs.value();
}

/**
 * @brief Represents a typed value. This can be used in the context of a variable (with a name), or as a constant (anonymous).
 */
class value {
public:
    value() = default;
    value(value const& rhs);
    value& operator=(value const& rhs);

    value(value&& rhs) noexcept;
    value& operator=(value&& rhs) noexcept;

    ~value();

    // Creates a new value with matching type, allocates memory for it and initializes it.

    static ps::value null();
    static ps::value from(ps::memory_pool& memory, int v);
    static ps::value from(ps::memory_pool& memory, unsigned int v);
    static ps::value from(ps::memory_pool& memory, float v);
    static ps::value from(ps::memory_pool& memory, bool v);
    static ps::value from(ps::memory_pool& memory, ps::list_type const& v);
    static ps::value from(ps::memory_pool& memory, ps::string_type const& v);
    static ps::value from(ps::memory_pool& memory, ps::struct_type const& v);
    static ps::value from(ps::memory_pool& memory, ps::external_type const& v);

    // construct a value as a reference, regardless of its type.
    static ps::value ref(ps::value const& rhs);

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

    template<typename T>
    T cast() const;

    friend std::ostream& operator<<(std::ostream& out, value const& v);

    void on_destroy();

    inline bool is_reference() const { return is_ref; }

private:
    mutable ps::memory_pool* memory = nullptr;

    // Pointer to allocated memory for this value.
    ps::pointer ptr = ps::null_pointer;
    type tpe{};
    // if is_ref is true, but refcount is null, this is an uncounted reference and shouldn't be cleaned up;
    std::shared_ptr<int> refcount = nullptr;
    bool is_ref = false;
};

template<typename F>
void visit_value(value& v, F&& callable) {
    switch(v.get_type()) {
        case type::null:
            break;
        case type::integer:
            callable(static_cast<ps::integer&>(v));
            break;
        case type::uint:
            callable(static_cast<ps::uint&>(v));
            break;
        case type::real:
            callable(static_cast<ps::real&>(v));
            break;
        case type::boolean:
            callable(static_cast<ps::boolean&>(v));
            break;
        case type::str:
            callable(static_cast<ps::str&>(v));
            break;
        case type::list:
            callable(static_cast<ps::list&>(v));
            break;
        case type::structure:
            callable(static_cast<ps::structure&>(v));
            break;
        case type::external:
            callable(static_cast<ps::external&>(v));
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
        case type::uint:
            callable(static_cast<ps::uint const&>(v));
            break;
        case type::real:
            callable(static_cast<ps::real const&>(v));
            break;
        case type::boolean:
            callable(static_cast<ps::boolean const&>(v));
            break;
        case type::str:
            callable(static_cast<ps::str const&>(v));
            break;
        case type::list:
            callable(static_cast<ps::list const&>(v));
            break;
        case type::structure:
            callable(static_cast<ps::structure const&>(v));
            break;
        case type::external:
            callable(static_cast<ps::external const&>(v));
            break;
        default:
            break;
    }
}

template<typename T>
T value::cast() const {
    T result = {};
    visit_value(*this, [&result](auto const& v) {
        result = static_cast<T>(v.value());
    });
    return result;
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

inline ps::value operator-(ps::value const& lhs) {
    ps::value result = ps::value::null();
    visit_value(lhs, [&lhs, &result](auto const& lhs_val) {
        result = value::from(lhs.get_memory(), -lhs_val);
    });
    return result;
}

inline ps::value operator!(ps::value const& lhs) {
    ps::value result = ps::value::null();
    visit_value(lhs, [&lhs, &result](auto const& lhs_val) {
        result = value::from(lhs.get_memory(), !lhs_val);
    });
    return result;
}

inline ps::value& operator++(ps::value& lhs) {
    visit_value(lhs, [&lhs](auto& lhs_val) {
        lhs_val = lhs_val + ps::integer { 1 };
    });
    return lhs;
}

inline ps::value& operator--(ps::value& lhs) {
    visit_value(lhs, [&lhs](auto& lhs_val) {
        lhs_val = lhs_val - ps::integer { 1 };
    });
    return lhs;
}


GEN_VALUE_OP(+)
GEN_VALUE_OP(-)
GEN_VALUE_OP(*)
GEN_VALUE_OP(/)
GEN_VALUE_OP(==)
GEN_VALUE_OP(!=)
GEN_VALUE_OP(&&)
GEN_VALUE_OP(||)
GEN_VALUE_OP(<)
GEN_VALUE_OP(>)
GEN_VALUE_OP(<=)
GEN_VALUE_OP(>=)
GEN_VALUE_OP(<<)
GEN_VALUE_OP(>>)
GEN_VALUE_OP(^)
GEN_VALUE_OP(&)
GEN_VALUE_OP(%)

// +=, -=, *=, /= etc
GEN_MUTABLE_OP(+)
GEN_MUTABLE_OP(-)
GEN_MUTABLE_OP(*)
GEN_MUTABLE_OP(/)
GEN_MUTABLE_OP(^)
GEN_MUTABLE_OP(&)
GEN_MUTABLE_OP(%)

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