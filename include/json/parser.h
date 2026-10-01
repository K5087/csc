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
    std::optional<type> parse_value(std::string_view view);

    friend std::optional<Object> parser_object(std::string_view view);

private:
    std::map<std::string_view, type> pairs;
};

std::optional<Object> parser_object(std::string_view view);

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
