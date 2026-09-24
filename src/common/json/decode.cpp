#include <json/decode.h>
#include <log/log.h>

#include <charconv>

#define DECODE_FUNC_IMPL(type)                                \
    bool decode(std::string_view view, type& value) {         \
        std::from_chars_result res =                          \
            std::from_chars(view.begin(), view.end(), value); \
        if (res.ec != std::errc()) {                          \
            loge("decode %s failed", typeid(type).name());    \
        }                                                     \
        return true;                                          \
    }

namespace json {
DECODE_FUNC_IMPL(int);

bool decode(std::string_view view, bool& value) {
    if (view == "true") {
        value = true;
    } else if (view == "false") {
        value = false;
    } else {
        loge("decode %s failed", typeid(bool).name());
        return false;
    }
    return true;
}

DECODE_FUNC_IMPL(float);
DECODE_FUNC_IMPL(double);

bool decode(std::string_view view, std::string& value) {
    if (view.starts_with("\"") && view.ends_with("\"") && view.size() > 2) {
        value = view.substr(1, view.size() - 2);
    } else {
        loge("decode %s failed", typeid(std::string).name());
        return false;
    }

    return true;
}

bool decode(std::string_view view, std::filesystem::path& value) {
    if (view.starts_with("\"") && view.ends_with("\"") && view.size() > 2) {
        value = view.substr(1, view.size() - 2);
    } else {
        loge("decode %s failed", typeid(std::string).name());
        return false;
    }
    return true;
}
} // namespace json
