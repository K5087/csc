#pragma once
#include <json/decode.h>
#include <log/log.h>

#include <map>
#include <string_view>
#include <variant>
#include <vector>

namespace json {
class Object;
class Array;

using type = std::variant<Object, Array, std::string_view>;

class Array {
    std::string_view array;
    std::vector<type> elements;
};

class Object {
public:
    std::optional<type> parse_value(std::string_view view) {
        std::size_t index = 0;
        while (index <= view.size()) {
            char c = view[index];
            switch (c) {
                case '{': break;
                case '[': break;
                case '"': {
                    std::size_t find = view.find('"', index + 1);
                    break;
                }
                default: break; index += 1;
            }
        }
    }

    friend std::optional<Object> parser_object(std::string_view view);

private:
    std::map<std::string_view, type> pairs;
};

std::optional<Object> parser_object(std::string_view view) {
    Object obj;
    std::size_t index = 0;
    while (index <= view.size()) {
        char c = view[index];
        switch (c) {
            case '{':
                loge("json object can't have object member");
                return std::nullopt;
            case '[':

                loge("json object can't have array member");
                return std::nullopt;
            case '"': {
                std::size_t quote = view.find('"', index + 1);
                if (quote == view.npos) {
                    loge("json object parse key failed");
                    return std::nullopt;
                }
                std::string_view key =
                    view.substr(index + 1, quote - index - 1);
                std::size_t colon = view.find(':', quote);
                if (colon == view.npos) {
                    loge("json objec parse key value failed");
                    return std::nullopt;
                }

                std::size_t value_begin = view.find_first_not_of(" \t\n\v\r");
                if (value_begin == view.npos) {
                    loge("json objec parse value failed");
                    return std::nullopt;
                }
                std::size_t object_end =
                    view.find_last_not_of('}', view.size());
                if (object_end == view.npos) {
                    loge("json objec parse failed");
                    return std::nullopt;
                }
                std::size_t value_end =
                    view.find_last_not_of(" \t\n\v\r", view.size() - 1);
                if (value_end == view.npos) {
                    loge("json objec parse value failed");
                    return std::nullopt;
                }
                auto value = obj.parse_value(
                    view.substr(value_begin, value_end - value_begin - 1));
                if (!value) { return std::nullopt; }
                auto [_, ret] = obj.pairs.insert({key, *value});
                if (!ret) {
                    loge("json objec insert key %s failed", key);
                    return std::nullopt;
                }
                break;
            }
            default: break; index += 1;
        }
    }

    return obj;
}

class Parser {
public:
    explicit Parser(std::string_view json);

    Parser& operator[](const std::string_view key) {
        while (index <= json.size()) {
            char c = json[index];
            switch (c) {
                case '{': break;
                case '[': break;
                case '"': {
                    std::size_t find = json.find('"', index + 1);
                    if (key == json.substr(index + 1, find - index - 1)) {
                    } else {
                        loge("parse json failed");
                        return *this;
                    }
                    break;
                }
                default: break; index += 1;
            }
        }
        return *this;
    }

private:
    void parser_array(std::string_view view);

    std::optional<std::string_view> parser_object(std::string_view view,
                                                  Object& obj,
                                                  std::string_view key = "");
    template <typename T>
    void parser_value(std::string_view view, T& t);

    bool parse(std::string_view view) {}

private:
    const std::string_view json;
    std::size_t index;
    type tree;
};

} // namespace json
