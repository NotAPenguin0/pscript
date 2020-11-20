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
}