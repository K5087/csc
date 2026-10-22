#pragma once
#define JSON_ERROR_HANDLE return false;
#include <json/json.h>

#include <filesystem>
#include <vector>

namespace csc {

namespace fs = std::filesystem;

namespace dep {

enum class Lookup_Method { by_name, include_angle, include_quote };

struct Require {
    std::string logical_name;
    std::optional<fs::path> compiled_module_path;
    std::optional<fs::path> source_path;
    bool unique_on_source_path = false;
    Lookup_Method lookup_method = Lookup_Method::by_name;
};

struct Provide {
    std::string logical_name;
    std::optional<fs::path> compiled_module_path;
    std::optional<fs::path> source_path;
    bool unique_on_source_path = false;
    bool is_interface = true;
};

struct Rule {
    std::optional<fs::path> primary_output;
    std::optional<fs::path> outputs;
    std::vector<Provide> provides = {};
    std::vector<Require> require_files = {};
};

struct Info {
    int version;
    int revision = 0;
    std::vector<Rule> rules;
};
} // namespace dep

std::vector<fs::path> parse_dep(const std::string& context,
                                const fs::path& root);
} // namespace csc

namespace json {
namespace dep = csc::dep;
bool encode(std::string& json, const dep::Lookup_Method value, encode_tag = {});
bool encode(std::string& json, const dep::Require& value, encode_tag = {});
bool encode(std::string& json, const dep::Provide& value, encode_tag = {});
bool encode(std::string& json, const dep::Rule& value, encode_tag = {});
bool encode(std::string& json, const dep::Info& value, encode_tag = {});

bool decode(const Value& value, dep::Lookup_Method& payload);
bool decode(const Value& value, dep::Require& payload);
bool decode(const Value& value, dep::Provide& payload);
bool decode(const Value& value, dep::Rule& payload);
bool decode(const Value& value, dep::Info& payload);

} // namespace json
