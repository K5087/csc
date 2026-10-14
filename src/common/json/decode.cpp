#include <json/decode.h>
#include <log/log.h>

#include <charconv>
#include <string>

#define DECODE_FUNC_IMPL(type)                                  \
    bool decode(const Value& value, type& payload) {            \
        auto view = value.get_view();                           \
        std::from_chars_result res =                            \
            std::from_chars(view.begin(), view.end(), payload); \
        if (res.ec != std::errc()) {                            \
            loge("decode %s failed: %s", typeid(type).name(),   \
                 std::string(view).c_str());                    \
        }                                                       \
        return true;                                            \
    }

namespace json {
DECODE_FUNC_IMPL(int);

bool decode(const Value& value, bool& payload) {
    auto view = value.get_view();
    if (view == "true") {
        payload = true;
    } else if (view == "false") {
        payload = false;
    } else {
        loge("decode %s failed: %s", typeid(bool).name(),
             std::string(view).c_str());
        return false;
    }
    return true;
}

DECODE_FUNC_IMPL(float);
DECODE_FUNC_IMPL(double);

bool decode(const Value& value, std::string& payload) {
    payload = value.get_view();
    return true;
}

bool decode(const Value& value, std::filesystem::path& payload) {
    payload = value.get_view();
    return true;
}
} // namespace json
