#pragma once
#include <algorithm>
#include <concepts>
#include <limits>

namespace math {
// TODO: remove when libc++ c++26 impl constexpr math
namespace temp {
template <std::floating_point T>
constexpr std::size_t min_decimal_exponent10() {
    T value = std::numeric_limits<T>::denorm_min();
    std::size_t exp = 0;

    while (value < T{1}) {
        value *= T{10};
        --exp;
    }

    return exp;
    // return static_cast<std::size_t>(
    //     -std::floor(std::log10(std::numeric_limits<T>::denorm_min())));
}
} // namespace temp

constexpr std::size_t decimal_digits(std::size_t n) {
    std::size_t digits = 1;

    while (n >= 10) {
        n /= 10;
        ++digits;
    }

    return digits;
}

template <std::floating_point T>
constexpr std::size_t max_decimal_exponent10 = [] {
    constexpr int min_exp = -math::temp::min_decimal_exponent10<T>();
    constexpr int max_exp = std::numeric_limits<T>::max_exponent10;

    return static_cast<std::size_t>(std::max(min_exp, max_exp));
}();

} // namespace math
