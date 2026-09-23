#include <json/encode.h>
#include <log/log.h>

#define ENCODE_FUNC_IMPL(type)                               \
    bool encode(std::string& json, build_type<type> value) { \
        json.append(std::to_string(value));                  \
        return true;                                         \
    }

namespace json {
ENCODE_FUNC_IMPL(int)

bool encode(std::string& json, const bool value) {
    json.append(value ? "true" : "false");
    return true;
}
ENCODE_FUNC_IMPL(float)
ENCODE_FUNC_IMPL(double)

bool encode(std::string& json, const std::filesystem::path& value) {
    json.append(value.generic_string());
    return true;
}

bool encode(std::string& json, const std::string& value) {
    json.reserve(json.size() + 2 + value.size());
    json.append("\"");
    json.append(value);
    json.append("\"");
    return true;
}
} // namespace json
