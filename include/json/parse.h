#pragma once
#include <json/decode.h>
#include <json/type.h>

#include <string_view>

namespace json {
std::optional<Value> parse(std::string_view json, std::size_t& pos);
std::optional<Value> parse_value(std::string_view json, std::size_t& pos);
std::optional<String> parse_string(std::string_view json, std::size_t& pos);
std::optional<Number> parse_number(std::string_view json, std::size_t& pos);
std::optional<Boolean> parse_boolean(std::string_view json, std::size_t& pos);
std::optional<Null> parse_null(std::string_view json, std::size_t& pos);
std::optional<Object> parse_object(std::string_view json, std::size_t& pos);
std::optional<std::pair<String, Value>> parse_member(std::string_view json,
                                                     std::size_t& pos);
std::optional<Array> parse_array(std::string_view json, std::size_t& pos);

std::optional<std::size_t> consume(Grammar grammar, std::string_view json,
                                   std::size_t& pos);

namespace rfc {
bool isdigit(char c);
bool isxdigit(char c);
constexpr bool isescape(char c) noexcept;
constexpr bool isws(char c) noexcept;
std::optional<std::string> escape(std::string_view view);
} // namespace rfc

} // namespace json
