#pragma once

#include <plib/types.hpp>

namespace plib {

namespace detail {

    template<typename T, template<typename, typename> typename F, typename... Options>
    struct find_best_match_impl;

    template<typename T, template<typename, typename> typename F, typename Cur>
    struct find_best_match_impl<T, F, Cur> {
        using type = Cur;
    };

    template<typename T, template<typename, typename> typename F, typename Cur, typename Head, typename... Next>
    struct find_best_match_impl<T, F, Cur, Head, Next...> {
        using type = std::conditional_t<F<T, Head>::value,
            typename find_best_match_impl<T, F, Head, Next...>::type,
            typename find_best_match_impl<T, F, Cur, Next...>::type>;
    };

    template<typename E, typename T>
    struct underlying_enum_type_filter {
        static constexpr bool value = sizeof(E) == sizeof(T) && (T(-1) < T(0) == E(-1) < E(0));
    };

} // namespace detail

template<typename E>
struct underlying_type {
    using type = typename detail::find_best_match_impl<E, detail::underlying_enum_type_filter,
        uint8_t, uint16_t, uint32_t, uint64_t, int8_t, int16_t, int32_t, int64_t
    >::type;
};

template<typename E>
using underlying_type_t = typename underlying_type<E>::type;

template<typename... Ts>
struct pack {};

namespace detail {

template<size_t N, typename... Ts>
struct make_pack_impl;

template<template<size_t, typename...> typename T, size_t N, typename... Args>
struct lazy_make_pack {
    using instantiated = T<N, Args...>;
};

template<
        template< // lazy_make_pack
        template<size_t, typename...> typename T_L, size_t N_L, typename... Args_L
> typename L,
        template<size_t, typename...> typename T, size_t N, typename... Args // real args
>
struct lazy_make_pack_type {
    using type = typename L<T, N, Args...>::instantiated::type;
};

template<typename... Ts>
struct pack_helper {
    using type = pack<Ts...>;
};

template<size_t N, typename T, typename... Ts>
struct make_pack_impl<N, T, Ts...> {
    using type_helper = std::conditional_t<N == 0,
            pack_helper<Ts...>,
            lazy_make_pack_type<lazy_make_pack, make_pack_impl, N - 1, T, T, Ts...>>;
    using type = typename type_helper::type;
};
} // namespace detail

template<size_t N, typename T>
struct make_pack {
    using type = typename detail::make_pack_impl<N, T>::type;
};


}