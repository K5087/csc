#pragma once
#include <filesystem>
#include <string>

/*
 *  only support base type, container not support, reduce template number
 */

#define ENCODE_KEY_VALUE(json, key, value) \
    json.append(key);                      \
    json.append(":");                      \
    encode(json, value);
#define ENCODE_KEY_ARRAY(json, key, array)                 \
    json.append(key);                                      \
    json.append(":[");                                     \
    for (auto& element : array) { encode(json, element); } \
    json.append("]");

#define ENCODE_FUNC(type) \
    bool encode(std::string& json, build_type<type> value);

namespace json {
template <typename T>
using build_type = std::conditional_t<(sizeof(T) <= 8), const T, const T&>;

ENCODE_FUNC(int);
ENCODE_FUNC(bool);
ENCODE_FUNC(float);
ENCODE_FUNC(double);
ENCODE_FUNC(std::string);
ENCODE_FUNC(std::filesystem::path);
} // namespace json

#undef ENCODE_FUNC
