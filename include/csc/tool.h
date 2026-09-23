#pragma once
#include <json/json.h>

#include <filesystem>
#include <optional>
#include <vector>

namespace csc {
namespace fs = std::filesystem;

namespace dep {
struct Require {
    std::string logic_name;
    fs::path source_path;
};

struct Provide {
    bool is_interface;
    std::string logic_name;
    fs::path source_path;
};

struct Rule {
    fs::path primary_output;
    std::vector<Provide> provides;
    std::vector<Require> require_files;
};

struct Info {
    int revision;
    std::vector<Rule> rules;
    int version;
};
} // namespace dep

std::vector<fs::path> find_file(const std::vector<fs::path>& path,
                                const std::string& extension);

std::vector<fs::path> find_file(const std::vector<fs::path>& path,
                                const std::vector<std::string>& extension);
std::optional<std::string> read_file(const fs::path& path);
bool write_file(const fs::path& path, const std::string& data);
std::vector<fs::path> parse_dep(const std::string& context,
                                const fs::path& root);
} // namespace csc

namespace json {

bool encode(std::string& json, const csc::dep::Require& value);
bool encode(std::string& json, const csc::dep::Provide& value);
bool encode(std::string& json, const csc::dep::Rule& value);
bool encode(std::string& json, const csc::dep::Info& value);
} // namespace json
