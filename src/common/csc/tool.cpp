#include <csc/tool.h>

#include <cassert>
#include <fstream>

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

namespace csc {

std::vector<fs::path> find_file(const std::vector<fs::path>& paths,
                                const std::string& extension) {
    std::vector<fs::path> result;

    for (const auto& root : paths) {
        if (!fs::exists(root)) { continue; }
        for (const auto& entry :
             std::filesystem::recursive_directory_iterator(root)) {
            if (entry.path().extension() == extension) {
                // result.push_back(absolute(entry.path()));
                result.push_back((entry.path()));
            }
        }
    }

    return result;
}

std::vector<fs::path> find_file(const std::vector<fs::path>& paths,
                                const std::vector<std::string>& extension) {
    std::vector<fs::path> result;

    for (const auto& root : paths) {
        if (!fs::exists(root)) { continue; }
        for (const auto& entry :
             std::filesystem::recursive_directory_iterator(root)) {
            if (std::find(extension.begin(), extension.end(),
                          entry.path().extension()) != extension.end()) {
                // result.push_back(absolute(entry.path()));
                result.push_back((entry.path()));
            }
        }
    }

    return result;
}

std::optional<std::string> read_file(const fs::path& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) { return std::nullopt; }

    const auto end = file.tellg();
    if (end < 0) { return std::nullopt; }

    std::string buff(static_cast<std::size_t>(end), '\0');

    file.seekg(0, std::ios::beg);

    if (!buff.empty()) {
        file.read(buff.data(), static_cast<std::streamsize>(buff.size()));

        if (!file) { return std::nullopt; }
    }

    return buff;
}

bool write_file(const fs::path& path, const std::string& data) {
    std::ofstream file(path, std::ios::binary);
    if (!file) { return false; }
    file << data;
    if (!file) { return false; }
    return true;
}

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
