#pragma once
#include <filesystem>
#include <string_view>

/*
 *  only support base type, container not support, reduce template number
 */

#define DECODE_FUNC(type) bool decode(std::string_view view, type& value);

namespace json {
DECODE_FUNC(int);
DECODE_FUNC(bool);
DECODE_FUNC(float);
DECODE_FUNC(double);
DECODE_FUNC(std::string);
DECODE_FUNC(std::filesystem::path);

} // namespace json

#undef DECODE_FUNC
