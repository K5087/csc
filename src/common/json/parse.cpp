#include <json/parse.h>
#include <log/log.h>

#include <charconv>
#define JSON_CHECK(boolean, msg) \
    if (boolean) {               \
        loge(msg);               \
        return std::nullopt;     \
    }

namespace json {
Type::Type(std::string_view view) : view(view) {}

std::optional<Value> parse(std::string_view json, std::size_t& pos) {
    consume(Grammar::ws, json, pos);
    if (pos == json.size()) { return Null{json}; }

    auto value = parse_value(json, pos);
    // consume(Grammar::ws,json, pos);

    if (value) {
        return *value;
    } else {
        loge("parse json failed");
        return std::nullopt;
    }
}

std::optional<Value> parse_value(std::string_view json, std::size_t& pos) {
    char c = json[pos];
    if (c == 'f' || c == 't') {
        return parse_boolean(json, pos);
    } else if (c == 'n') {
        return parse_null(json, pos);
    } else if (c == '{') {
        return parse_object(json, pos);
    } else if (c == '[') {
        return parse_array(json, pos);
    } else if (c == '"') {
        return parse_string(json, pos);
    } else if (c == '-' || rfc::isdigit(c)) {
        return parse_number(json, pos);
    } else {
        throw std::runtime_error("unexcept json value");
    }
}

std::optional<Object> parse_object(std::string_view json, std::size_t& pos) {
    auto begin = consume(Grammar::begin_objct, json, pos).value_or(json.size());
    JSON_CHECK(begin == json.size() || pos == json.size(),
               "parse object failed");

    Object obj{json};
    auto member = parse_member(json, pos);
    while (member) {
        auto [it, success] = obj.pairs.emplace(std::move(*member));
        if (!success) {
            loge("object memeber have same key");
            return std::nullopt;
        }
        if (!consume(Grammar::value_separator, json, pos)) break;
        JSON_CHECK(pos == json.size(), "parse object member failed");
        member = parse_member(json, pos);
    }

    auto end = consume(Grammar::end_object, json, pos).value_or(json.size());
    JSON_CHECK(end == json.size(), "parse object failed");

    obj.view = json.substr(begin, end - begin + 1);
    return obj;
} // namespace jso

std::optional<std::pair<String, Value>> parse_member(std::string_view json,
                                                     std::size_t& pos) {
    auto string = parse_string(json, pos);
    if (!string) return std::nullopt;
    if (!consume(Grammar::name_separator, json, pos)) return std::nullopt;
    JSON_CHECK(pos == json.size(), "parse object member failed");
    auto value = parse_value(json, pos);
    JSON_CHECK(!value, "parse object member failed");
    return std::pair{*string, *value};
}

std::optional<Array> parse_array(std::string_view json, std::size_t& pos) {
    auto begin = consume(Grammar::begin_array, json, pos).value_or(json.size());
    JSON_CHECK(begin == json.size() || pos == json.size(),
               "parse array failed");

    Array array{json};
    auto value = parse_value(json, pos);
    while (value) {
        array.values.push_back(std::move(*value));
        if (!consume(Grammar::value_separator, json, pos)) break;
        JSON_CHECK(pos == json.size(), "parse array values failed");
        value = parse_value(json, pos);
    }

    auto end = consume(Grammar::end_array, json, pos).value_or(json.size());
    JSON_CHECK(end == json.size(), "parse array failed");

    array.view = json.substr(begin, end - begin + 1);
    return array;
}

std::optional<Number> parse_number(std::string_view json, std::size_t& pos) {
    auto index = pos;

    // consume minus
    bool minus = json[pos] == '-';
    if (minus) {
        pos++;
        JSON_CHECK(pos == json.size(), "parse number failed");
    }

    // consume int
    if (json[pos] == '0') {
        pos++;
        JSON_CHECK(pos < json.size() && rfc::isdigit(json[pos]),
                   "parse number failed");
    } else {
        while (pos < json.size() && rfc::isdigit(json[pos])) { pos++; }
    }

    // parse frac
    if (pos == json.size()) { goto defer; }

    if (json[pos] == '.') {
        pos++;
        JSON_CHECK(pos >= json.size(), "parse number failed");
        while (pos < json.size() && rfc::isdigit(json[pos])) { pos++; }
    }

    // parse exp
    if (pos == json.size()) { goto defer; }

    if (json[pos] == 'e' || json[pos] == 'E') {
        pos++;
        JSON_CHECK(pos < json.size(), "parse number failed");
        if (json[pos] == '+' || json[pos] == '-') {
            pos++;
            JSON_CHECK(pos < json.size(), "parse number failed");
        }
        while (pos < json.size() && rfc::isdigit(json[pos])) { pos++; }
    }

defer:
    double number;
    auto ret =
        std::from_chars(json.data() + index, json.data() + json.size(), number);
    JSON_CHECK(ret.ec != std::errc{}, "parse number failed");
    auto length = ret.ptr - json.data() - index;
    return Number{json.substr(index, length)};
}

std::optional<String> parse_string(std::string_view json, std::size_t& pos) {
    JSON_CHECK(json[pos] != '"', "parse string failed");

    // consume char
    pos++;
    std::size_t begin = pos;
    while (pos < json.size() && json[pos] != '"') {
        if (json[pos] == '\\') {
            pos++;
            JSON_CHECK(pos == json.size(), "parse string failed");
            if (json[pos] == 'u') {
                pos++;
                for (int i = 0; i < 4; ++i) {
                    JSON_CHECK(pos >= json.size() || !rfc::isxdigit(json[pos]),
                               "invalid unicode escape");
                    ++pos;
                }
                continue;
            } else if (rfc::isescape(json[pos])) {
                pos++;
                continue;
            } else {
                loge("parse string failed");
                return std::nullopt;
            }
        } else if (static_cast<unsigned char>(json[pos]) < 0x20) {
            loge("parse string failed");
            return std::nullopt;
        }
        ++pos;
    }

    JSON_CHECK(pos == json.size(), "parse string failed");

    std::size_t end = pos - 1;
    pos++;
    return String{json.substr(begin, end - begin + 1)};
}

std::optional<Boolean> parse_boolean(std::string_view json, std::size_t& pos) {
    std::string_view text = json.substr(pos, 5);
    if (text.starts_with("true")) {
        pos += 4;
        return Boolean(text.substr(0, 4));
    } else if (text.starts_with("false")) {
        pos += 5;
        return Boolean(text);
    } else {
        loge("parse boolean failed");
        return std::nullopt;
    }
}

std::optional<Null> parse_null(std::string_view json, std::size_t& pos) {
    std::string_view text = json.substr(pos, 4);
    if (text == "null") {
        return Null(text);
    } else {
        loge("parse null failed");
        return std::nullopt;
    }
}

#define CONSUME_GRAMMAR(c, msg)                            \
    consume(Grammar::ws, json, pos);                       \
    JSON_CHECK(pos == json.size() || json[pos] != c, msg); \
    auto index = pos;                                      \
    pos++;                                                 \
    consume(Grammar::ws, json, pos);                       \
    return index;

std::optional<std::size_t> consume(Grammar grammar, std::string_view json,
                                   std::size_t& pos) {
    switch (grammar) {
        case Grammar::ws:
            while (pos < json.size() && rfc::isws(json[pos])) { pos++; }
            return std::nullopt;
        case Grammar::value:
            loge("should not consume value");
            return std::nullopt;

        case Grammar::begin_array: {
            CONSUME_GRAMMAR('[', "consume array begin failed");
        }
        case Grammar::begin_objct: {
            CONSUME_GRAMMAR('{', "consume object begin failed");
        }
        case Grammar::end_array: {
            CONSUME_GRAMMAR(']', "consume array end failed");
        }
        case Grammar::end_object: {
            CONSUME_GRAMMAR('}', "consume obejct end failed");
        }
        case Grammar::name_separator: {
            consume(Grammar::ws, json, pos);
            JSON_CHECK(pos == json.size(), "consume name separator failed");
            if (json[pos] != ':') { return std::nullopt; };
            auto index = pos;
            pos++;
            consume(Grammar::ws, json, pos);
            return index;
        }
        case Grammar::value_separator: {
            consume(Grammar::ws, json, pos);
            JSON_CHECK(pos == json.size(), "consume value separator failed");
            if (json[pos] != ',') { return std::nullopt; };
            auto index = pos;
            pos++;
            consume(Grammar::ws, json, pos);
            return index;
        }
        default: throw std::runtime_error("unexcept json grammar");
    }
}

namespace rfc {
bool isdigit(char c) { return std::isdigit(static_cast<unsigned char>(c)); }

bool isxdigit(char c) { return std::isxdigit(static_cast<unsigned char>(c)); }

constexpr bool isescape(char c) noexcept {
    return std::string_view{"\"\\/bfnrt"}.contains(c);
}

constexpr bool isws(char c) noexcept {
    return std::string_view{" \t\r\n"}.contains(c);
}

} // namespace rfc

} // namespace json
