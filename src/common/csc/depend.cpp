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
} // namespace csc

namespace json {
bool encode(std::string& json, const csc::dep::Require& value) {
    ENCODE_KEY_VALUE(json, "logic-name", value.logic_name);
    json.append(",");
    ENCODE_KEY_VALUE(json, "source-path", value.source_path);
    return true;
}

bool encode(std::string& json, const csc::dep::Provide& value) {
    ENCODE_KEY_VALUE(json, "is-interface", value.is_interface);
    json.append(",");
    ENCODE_KEY_VALUE(json, "logic-name", value.logic_name);
    json.append(",");
    ENCODE_KEY_VALUE(json, "source-path", value.source_path);
    return true;
}

bool encode(std::string& json, const csc::dep::Rule& value) {
    ENCODE_KEY_VALUE(json, "primary-output", value.primary_output);
    json.append(",");
    ENCODE_KEY_ARRAY(json, "provides", value.provides);
    json.append(",");
    ENCODE_KEY_ARRAY(json, "requires", value.require_files);
    return true;
}

bool encode(std::string& json, const csc::dep::Info& value) {
    ENCODE_KEY_VALUE(json, "revision", value.revision);
    json.append(",");
    ENCODE_KEY_ARRAY(json, "rules", value.rules);
    json.append(",");
    ENCODE_KEY_VALUE(json, "version", value.version);
    return true;
}

} // namespace json
