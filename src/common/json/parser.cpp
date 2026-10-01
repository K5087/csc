#include <json/parser.h>
#define JSON_ASSERT(boolean, msg) \
    if (boolean) {                \
        loge(msg);                \
        return std::nullopt;      \
    }

namespace json {
std::optional<type> parse_value(std::string_view view) {
    std::size_t index = 0;
    while (index <= view.size()) {
        char c = view[index];
        switch (c) {
            case '{': {
                std::size_t end = view.find('"', index + 1);
                JSON_ASSERT(end == view.npos, "parse object failed");
                auto obj = parser_object(view.substr(index, end - index + 1));
                if (obj) { return *obj; }
                break;
            }
            case '[': break;
            case '"': {
                std::size_t find = view.find('"', index + 1);
                break;
            }
            default: break; index += 1;
        }
    }
}

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
} // namespace json
