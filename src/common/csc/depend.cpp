#include <csc/depend.h>

namespace csc {
std::vector<fs::path> parse_dep(const std::string& data, const fs::path& root) {
    std::vector<fs::path> paths;

    auto iter = data.begin();
    std::string str;
    auto push = [&]() {
        if (!str.empty()) {
            paths.push_back(root / str);
            str.clear();
        }
    };
    while (iter != data.end()) {
        switch (*iter) {
            case ':':
                if (std::next(iter) != data.end() && *std::next(iter) == ' ') {
                    str.clear();
                    iter++;
                    break;
                }
                str += *iter;
                break;
            case ' ': push(); break;
            case '\r': push(); break;
            case '\n': push(); break;
            case '\\': {
                auto next = std::next(iter);
                if (next == data.end()) {
                    str += *iter;
                    break;
                }

                switch (*next) {
                    case '\n': ++iter; break;
                    case '\r': {
                        auto next2 = std::next(next);
                        if (next2 != data.end() && *next2 == '\n')
                            iter += 2;
                        else
                            str += *iter;
                        break;
                    }
                    default: str += *iter; break;
                }
                break;
            }
            default: str += *iter;
        }
        iter++;
    }
    push();
    return paths;
}

dep::Info parse_p1689(const std::string& context) {
    std::size_t pos = 0;
    auto ret = json::parse(context, pos);
    if (!ret) throw std::runtime_error("parse_p1689 failed");
    dep::Info info;
    if (!decode(*ret, info)) { throw std::runtime_error("decode json failed"); }
    return info;
}
} // namespace csc

namespace json {

bool encode(std::string& json, const dep::Lookup_Method value, encode_tag) {
    switch (value) {
        case dep::Lookup_Method::by_name: json.append("by-name"); break;
        case dep::Lookup_Method::include_angle:
            json.append("include-angle");
            break;
        case dep::Lookup_Method::include_quote:
            json.append("include-quote");
            break;
        default: throw json_error("unexpected lookup-method value");
    }
    return true;
}

bool encode(std::string& json, const dep::Require& value, encode_tag) {
    json.append("{");
    ENCODE_KEY_VALUE(json, "logical-name", value.logical_name);
    json.append(",");
    encode_key_optional(json, "compiled-module-path",
                        value.compiled_module_path);
    json.append(",");
    encode_key_optional(json, "source-path", value.source_path);
    json.append(",");
    ENCODE_KEY_OPTIONAL(json, "unique-on-source-path",
                        value.unique_on_source_path,
                        value.unique_on_source_path);
    json.append(",");
    ENCODE_KEY_OPTIONAL(json, "lookup-method", value.lookup_method,
                        value.lookup_method != dep::Lookup_Method::by_name);
    json.append("}");
    return true;
}

bool encode(std::string& json, const dep::Provide& value, encode_tag) {
    json.append("{");
    ENCODE_KEY_VALUE(json, "logical-name", value.logical_name);
    json.append(",");
    encode_key_optional(json, "compiled-module-path",
                        value.compiled_module_path);
    json.append(",");
    encode_key_optional(json, "source-path", value.source_path);
    json.append(",");
    ENCODE_KEY_OPTIONAL(json, "unique-on-source-path",
                        value.unique_on_source_path,
                        value.unique_on_source_path);
    json.append(",");
    ENCODE_KEY_OPTIONAL(json, "is-interface", value.is_interface,
                        !value.is_interface);
    json.append("}");
    return true;
}

bool encode(std::string& json, const dep::Rule& value, encode_tag) {
    json.append("{");
    encode_key_optional(json, "primary-output", value.primary_output);
    json.append(",");
    encode_key_optional(json, "outputs", value.outputs);
    json.append(",");
    ENCODE_KEY_OPTIONAL(json, "provides", value.provides,
                        !value.provides.empty());
    json.append(",");

    ENCODE_KEY_OPTIONAL(json, "requires", value.require_files,
                        !value.require_files.empty());
    json.append("}");
    return true;
}

bool encode(std::string& json, const dep::Info& value, encode_tag) {
    json.append("{");
    ENCODE_KEY_VALUE(json, "version", value.version);
    json.append(",");
    ENCODE_KEY_OPTIONAL(json, "revision", value.revision, value.revision == 0);
    json.append(",");
    ENCODE_KEY_VALUE(json, "rules", value.rules);
    json.append("}");
    return true;
}

bool decode(const Value& value, dep::Lookup_Method& payload) {
    auto string = value.get_string();
    if (string.view == "by-name") {
        payload = dep::Lookup_Method::by_name;
    } else if (string.view == "include-angle") {
        payload = dep::Lookup_Method::include_angle;
    } else if (string.view == "include-quote") {
        payload = dep::Lookup_Method::include_angle;
    } else {
        throw json_error("unexpected lookup-method value");
    }
    return true;
}

bool decode(const Value& value, dep::Require& payload) {
    auto object = value.get_object();
    DECODE_KEY_VALUE(object, "logical-name", payload.logical_name);
    decode_key_optional(object, "compiled-module-path",
                        payload.compiled_module_path);
    decode_key_optional(object, "source-path", payload.source_path);
    DECODE_KEY_OPTIONAL(object, "unique-on-source-path",
                        payload.unique_on_source_path);
    DECODE_KEY_OPTIONAL(object, "lookup-method", payload.lookup_method);
    return true;
}

bool decode(const Value& value, dep::Provide& payload) {
    auto object = value.get_object();
    DECODE_KEY_VALUE(object, "logical-name", payload.logical_name);
    decode_key_optional(object, "compiled-module-path",
                        payload.compiled_module_path);
    decode_key_optional(object, "source-path", payload.source_path);
    DECODE_KEY_OPTIONAL(object, "unique-on-source-path",
                        payload.unique_on_source_path);
    DECODE_KEY_OPTIONAL(object, "is-interface", payload.is_interface);
    return true;
}

bool decode(const Value& value, dep::Rule& payload) {
    auto object = value.get_object();
    decode_key_optional(object, "primary-output", payload.primary_output);
    decode_key_optional(object, "outputs", payload.outputs);
    DECODE_KEY_OPTIONAL(object, "provides", payload.provides);
    DECODE_KEY_OPTIONAL(object, "requires", payload.require_files);
    return true;
}

bool decode(const Value& value, dep::Info& payload) {
    auto object = value.get_object();
    DECODE_KEY_VALUE(object, "version", payload.version);
    DECODE_KEY_OPTIONAL(object, "revision", payload.revision);
    DECODE_KEY_VALUE(object, "rules", payload.rules);
    return true;
}
} // namespace json
