#pragma once

#include <plib/traits.hpp>
#include <functional>

namespace plib {

/**
 * @brief Callable type-erased function. V is the variant type used for values. Max amount of arguments supported is 8.
 * @tparam V variant type for values. Needs to have a static_cast<T> defined to access types.
 */
template<typename V>
class erased_function {
public:
    virtual V call() { return V {}; }
    virtual V call(V v) { return V {}; }
    virtual V call(V v1, V v2) { return V {}; }
    virtual V call(V v1, V v2, V v3) { return V {}; }
    virtual V call(V v1, V v2, V v3, V v4) { return V {}; }
    virtual V call(V v1, V v2, V v3, V v4, V v5) { return V {}; }
    virtual V call(V v1, V v2, V v3, V v4, V v5, V v6) { return V {}; }
    virtual V call(V v1, V v2, V v3, V v4, V v5, V v6, V v7) { return V {}; }
    virtual V call(V v1, V v2, V v3, V v4, V v5, V v6, V v7, V v8) { return V {}; }

    // call function with no return type
    virtual void call_void() {}
    virtual void call_void(V v1) {}
    virtual void call_void(V v1, V v2) {}
    virtual void call_void(V v1, V v2, V v3) {}
    virtual void call_void(V v1, V v2, V v3, V v4) {}
    virtual void call_void(V v1, V v2, V v3, V v4, V v5) {}
    virtual void call_void(V v1, V v2, V v3, V v4, V v5, V v6) {}
    virtual void call_void(V v1, V v2, V v3, V v4, V v5, V v6, V v7) {}
    virtual void call_void(V v1, V v2, V v3, V v4, V v5, V v6, V v7, V v8) {}

    virtual ~erased_function() = default;
};

template<typename F, typename V, typename ValuesPack>
class concrete_function;

namespace detail {

template<typename R, typename Args, typename Values>
struct call_func;

template<typename R, typename... Args, typename... Values>
struct call_func<R, pack<Args...>, pack<Values...>> {

    R (* function)(Args...) = nullptr;

    R operator()(Values& ... values) {
        return function(static_cast<Args>(values) ...);
    }
};

// Specialization for no return type
template<typename... Args, typename... Values>
struct call_func<void, pack<Args...>, pack<Values...>> {
    void (* function)(Args...) = nullptr;

    void operator()(Values& ... values) {
        return function(static_cast<Args>(values) ...);
    }
};

}

/**
 * @brief Stores a concrete function with argument types.
 * @tparam V Value type used for passing arguments to call().
 * @tparam R Return type of the function
 * @tparam Args Arguments to the function.
 * @tparam Values Pack of sizeof...(Args) instances of V. Provided if you use make_concrete_function
 */
template<typename V, typename R, typename... Args, typename... Values>
class concrete_function<R(Args...), V, pack<Values...>> : public erased_function<V> {
public:
    static_assert(sizeof... (Args) == sizeof... (Values), "Size must match");

    template<typename CreateFunc>
    concrete_function(R (*f)(Args...), CreateFunc&& c) {
        function = f;
        create_value = c;
    }

    ~concrete_function() override = default;

    V operator()(Values... values) {
        return call(values...);
    }

    V call(Values... values) override {
        detail::call_func<R, pack<Args...>, typename make_pack<sizeof...(Args), V>::type> caller { function };
        return create_value(caller(values...));
    }

    R (*function)(Args...) = nullptr;
    std::function<V(R const&)> create_value;
};

template<typename V, typename... Args, typename... Values>
class concrete_function<void(Args...), V, pack<Values...>> : public erased_function<V> {
public:
    static_assert(sizeof... (Args) == sizeof... (Values), "Size must match");

    // create func parameter ignored
    template<typename CreateFunc>
    concrete_function(void (*f)(Args...), CreateFunc&&) {
        function = f;
    }

    ~concrete_function() override = default;

    V operator()(Values... values) {
        return call(values...);
    }

    V call(Values... values) override {
        detail::call_func<void, pack<Args...>, typename make_pack<sizeof...(Args), V>::type> caller { function };
        // call the function
        caller(values...);
        // return default value
        return V {};
    }

    void (*function)(Args...) = nullptr;
};

/**
 * @brief Create a concrete function used for type erasing functions.
 * @tparam V Value type for passing arguments to call().
 * @tparam R Return type of the function.
 * @tparam C Type of the function used to create value types.
 * @tparam Args Argument types to the function.
 * @param function Function to wrap in the object.
 * @param create_func Callback to create a V from a type R.
 * @return Pointer to concrete_function object wrapping the given function. Allocated using new().
 */
template<typename V, typename R, typename C, typename... Args>
auto make_concrete_function(R (*function)(Args...), C&& create_func) {
    return new concrete_function<R(Args...), V, typename make_pack<sizeof...(Args), V>::type>(function, std::forward<C>(create_func));
}

}