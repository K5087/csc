#include <json/encode.h>
#include <log/log.h>
#include <math/math.h>

#include <charconv>
#include <concepts>
#include <typeinfo>

#define ENCODE_FUNC_IMPL(type)                                     \
    bool encode(std::string& json, build_type<type> value) {       \
        bool status = true;                                        \
        std::size_t old = json.size();                             \
                                                                   \
        json.resize_and_overwrite(                                 \
            old + max_chars<type>,                                 \
            [old, &value, &status](char* data, std::size_t size) { \
                std::to_chars_result ret =                         \
                    std::to_chars(data + old, data + size, value); \
                if (ret.ec != std::errc()) {                       \
                    loge("encode %s failed", typeid(type).name()); \
                    status = false;                                \
                    return old;                                    \
                }                                                  \
                return static_cast<std::size_t>(ret.ptr - data);   \
            });                                                    \
                                                                   \
        return status;                                             \
    }

namespace json {
template <typename T>
constexpr std::size_t max_chars = 0;

template <std::integral T>
constexpr std::size_t max_chars<T> =
    std::numeric_limits<T>::digits10 + 1 + std::is_signed_v<T>;

template <std::floating_point T>
constexpr std::size_t max_chars<T> =
    std::numeric_limits<T>::max_digits10 +
    math::decimal_digits(math::max_decimal_exponent10<T>) + 3 +
    std::is_signed_v<T>;

ENCODE_FUNC_IMPL(int)

bool encode(std::string& json, const bool value) {
    json.append(value ? "true" : "false");
    return true;
}
ENCODE_FUNC_IMPL(float)
ENCODE_FUNC_IMPL(double)

bool encode(std::string& json, const std::string& value) {
    json.reserve(json.size() + 2 + value.size());
    json.append("\"");
    json.append(value);
    json.append("\"");
    return true;
}

bool encode(std::string& json, const std::filesystem::path& value) {
    encode(json, value.generic_string());
    return true;
}
} // namespace json
