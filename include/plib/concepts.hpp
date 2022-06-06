#pragma once

#include <type_traits>
#include <concepts>

namespace plib {


template<typename T, typename U>
concept adds = requires(std::remove_reference_t<T> const& lhs, std::remove_reference_t<U> const& rhs) {
    { lhs + rhs } -> std::same_as<std::common_type_t<T, U>>;
};

template<typename T, typename U>
concept subtracts = requires(std::remove_reference_t<T> const& lhs, std::remove_reference_t<U> const& rhs) {
    { lhs - rhs } -> std::same_as<std::common_type_t<T, U>>;
};

template<typename T, typename U>
concept multiplies = requires(std::remove_reference_t<T> const& lhs, std::remove_reference_t<U> const& rhs) {
    { lhs * rhs } -> std::same_as<std::common_type_t<T, U>>;
};

template<typename T, typename U>
concept divides = requires(std::remove_reference_t<T> const& lhs, std::remove_reference_t<U> const& rhs) {
    { lhs / rhs } -> std::same_as<std::common_type_t<T, U>>;
};

}