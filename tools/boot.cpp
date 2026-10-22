#include <cmd/cmd.h>
#include <csc/csc.hpp>
#include <csc/debug.hpp>

#include <ranges>

auto current = CURRENT_DIR;
#ifdef _WIN32
std::string platform = "windows";
std::vector<std::string> flags = {"-luuid", "-lole32"};
#else
std::string platform = "unix";
std::vector<std::string> flags = {"-luuid", "-lole32"};
#endif

int main(int argc, char* argv[]) {
    csc::update_self(argc, argv, {__FILE__});
    auto sources = csc::find_file({current / "../src/common/"}, ".cpp");
    sources.append_range(
        csc::find_file({current / "../src" / platform}, ".cpp"));
    cmd::Cmd cmd{"clang++"};
    std::vector<std::string> units;
    units.reserve(sources.size());
    units.append_range(std::views::transform(sources, [](auto& path) {
        return std::filesystem::relative(path, current / "../")
            .generic_string();
    }));
    cmd.append_range(units);
    cmd.append_range(std::vector<std::string_view>{
        "build.cpp", "-Iinclude", "-o", "build.exe", "-std=c++26"});
    cmd.append_range(flags);
    print_cmd(cmd);
    return 0;
}
