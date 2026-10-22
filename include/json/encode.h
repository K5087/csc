#pragma once
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

/*
 *  only support base type, container not support, reduce template number
 */
#ifdef JSON_ERROR_HANDLE
    #define ENCODE_KEY_VALUE(json, key, value)        \
        if (!encode(json, key)) { JSON_ERROR_HANDLE } \
        json.append(":");                             \
        if (!encode(json, value)) { JSON_ERROR_HANDLE }

#else
    #define ENCODE_KEY_VALUE(json, key, value) \
        encode(json, key);                     \
        json.append(":");                      \
        encode(json, value);

#endif
#define ENCODE_KEY_OPTIONAL(json, key, value, expr) \
    if (expr) { ENCODE_KEY_VALUE(json, key, value) }

#define ENCODE_FUNC(type) \
    bool encode(std::string& json, build_type<type> value, encode_tag = {});

namespace json {
// for template adl
struct encode_tag {};

template <typename T>
using build_type = std::conditional_t<(sizeof(T) <= 8), const T, const T&>;

ENCODE_FUNC(int);
ENCODE_FUNC(bool);
ENCODE_FUNC(float);
ENCODE_FUNC(double);
ENCODE_FUNC(std::string);
ENCODE_FUNC(std::filesystem::path);
bool encode(std::string& json, const char* value, encode_tag = {});

template <class T>
bool encode(std::string& json, const std::vector<T>& array, encode_tag = {}) {
    json.append("[");

    bool first = true;
    for (const auto& element : array) {
        if (first) {
            first = false;
        } else {
            json.append(",");
        }
        if (!encode(json, element, encode_tag{})) { return false; }
    }

    json.append("]");
    return true;
}

template <typename T>
bool encode_key_optional(std::string& json, const std::string& key,
                         const std::optional<T>& value) {
    if (value) {
        if (!encode(json, key, encode_tag{})) { return false; }
        json.append(":");
        if (!encode(json, *value, encode_tag{})) { return false; }
    }
    return true;
}
} // namespace json

#undef ENCODE_FUNC
