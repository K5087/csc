#pragma once
#include <json/type.h>

#include <filesystem>
#include <string_view>

/*
 *  only support base type, container not support, reduce template number
 */

#define DECODE_FUNC(type) bool decode(const Value& value, type& payload);

namespace json {
DECODE_FUNC(int);
DECODE_FUNC(bool);
DECODE_FUNC(float);
DECODE_FUNC(double);
DECODE_FUNC(std::string);
DECODE_FUNC(std::filesystem::path);

template <class T>
bool decode(const Value& value, std::vector<T>& payload) {
    auto& array = value.get_array();
    for (auto& i : array) {
        T tmp;
        if (!decode(i, tmp)) { return false; }
        payload.push_back(std::move(tmp));
    }
    return true;
}

template <typename T>
bool decode_key_optional(const Object& obj, std::string_view key,
                         std::optional<T>& value) {
    if (auto payload = obj.find(key); payload) {
        if (!decode(*payload, value.emplace())) { return false; }
    } else {
        value = std::nullopt;
    }
    return true;
}

} // namespace json

#undef DECODE_FUNC
