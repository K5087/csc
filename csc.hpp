#include <cstdarg>
#include <expected>
#include <filesystem>
#include <fstream>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include <cstring>
#include <system_error>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <errno.h>
    #include <unistd.h>
#endif // _Win32

namespace csc {
using Dir  = std::filesystem::path;
using Path = std::filesystem::path;

template <typename T>
using Result = std::expected<T, std::string>;
using Reason = std::unexpected<std::string>;

namespace OS {
enum class System {
    linux,
    windows,
    macos,
};

inline bool is_terminal() {
#ifdef _WIN32
    return GetFileType(GetStdHandle(STD_OUTPUT_HANDLE)) == FILE_TYPE_CHAR;
#else
    return isatty(fileno(stderr));
#endif // _WIN32
}

inline Result<std::vector<char>> ReadFile(const Path& path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);

    if (!file) {
        return Reason("failed to open file!");
    }
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();

    std::vector<char> buffer(size);

    file.seekg(0);
    file.read(buffer.data(), size);
    file.close();
    return buffer;
}
} // namespace OS

namespace predefine {
#if defined __clang__
static std::string current_compiler = "clang++";
#elif define __GUN__
static std::string current_compiler = "gcc";
#endif // __clang__

} // namespace predefine

namespace log_impl {
enum Level {
    code,
    info,
    warn,
    erro,
};

enum class Color {
    red,
    green,
    yellow,
    blue,
    magenta,
    cyan,
    white,
};

inline std::string get_ansi_color(Color color) {
    switch (color) {
        case Color::red: return "31";
        case Color::green: return "32";
        case Color::yellow: return "33";
        case Color::blue: return "34";
        case Color::magenta: return "35";
        case Color::cyan: return "36";
        case Color::white: return "37";
        default: return "0";
    }
}

inline std::string colorize_str(const std::string_view str, Color color) {
    if (OS::is_terminal()) {
        std::string result;
        result.reserve(str.size() + 16);
        result += "\x1b[";
        result += get_ansi_color(color);
        result += "m";
        result += str;
        result += "\x1b[0m";
        return result;
    } else {
        return std::string(str);
    }
}

inline void log_color(Level level, const std::string& fmt, va_list list) {
    switch (level) {
        case Level::code: std::fprintf(stderr, "%s", colorize_str("[code]", Color::blue).c_str()); break;
        case Level::info: std::fprintf(stderr, "%s", colorize_str("[info]", Color::green).c_str()); break;
        case Level::warn: std::fprintf(stderr, "%s", colorize_str("[warn]", Color::yellow).c_str()); break;
        case Level::erro: std::fprintf(stderr, "%s", colorize_str("[erro]", Color::red).c_str()); break;
    }
    std::vfprintf(stderr, fmt.c_str(), list);
    std::fprintf(stderr, "\n");
}

#define CODE log_impl::Level::code
#define INFO log_impl::Level::info
#define WARN log_impl::Level::warn
#define ERRO log_impl::Level::erro
} // namespace log_impl

inline void log(log_impl::Level level, std::string_view fmt, ...) {
    va_list list;
    va_start(list, fmt);
    log_color(level, std::string(fmt), list);
    va_end(list);
}

struct DepInfo {
    std::vector<Path> targets;
    std::vector<Path> depends;
    DepInfo() = default;

    DepInfo(std::vector<std::string>& target, std::vector<std::string>& depend) : targets(target.begin(), target.end()), depends(depend.begin(), depend.end()) {
    };
};

class Translation_Unit {
public:
    Path path;
    Path obj;

    Translation_Unit(Path path) : path(path) {};
};

class Cmd {
public:
    template <typename... T>
    Cmd(T&&... t) { (AppendDispatch(std::forward<T>(t)), ...); }

public:
    template <typename... T>
    void Append(T&&... t) { (AppendDispatch(std::forward<T>(t)), ...); }

    void AppendRange(int argc, char** argv) {
        params.reserve(params.size() + argc);
        for (int i = 0; i < argc; i++) {
            params.emplace_back(argv[i]);
        }
    }

    void Clear() { params.clear(); };

    std::string GetCommandStr() const {
        std::string Command;
        for (size_t i = 0; i < params.size(); i++) {
            std::string_view arg = params[i];
            if (i > 0) Command.append(" ");
            if (!arg.empty() && std::string_view::npos == arg.find_first_of(" \t\n\v\"")) {
                Command.append(arg);
            } else {
                Command.append("\"");
                size_t backslashes = 0;
                for (size_t j = 0; j < arg.length(); ++j) {
                    switch (arg[j]) {
                        case '\\':
                            backslashes += 1;
                            break;
                        case '\"':
                            Command.append(2 * backslashes + 1, '\\');
                            backslashes = 0;
                            Command.push_back(arg[j]);
                            break;
                        default:
                            Command.append(backslashes, '\\');
                            backslashes = 0;
                            Command.push_back(arg[j]);
                            break;
                    }
                }
                Command.append(2 * backslashes, '\\');
                Command.append("\"");
            }
        }
        return Command;
    }

    bool empty() const {
        return params.empty();
    }

private:
    std::vector<std::string> params;

private:
    void AppendDispatch(const char* s) { params.emplace_back(s); }

    void AppendDispatch(const std::string& s) { params.emplace_back(s); }

    void AppendDispatch(std::string_view s) { params.emplace_back(s); }

    void AppendDispatch(const Path& p) {
        params.emplace_back(p.generic_string());
    }

    void AppendDispatch(const std::vector<Path>& paths) {
        params.reserve(params.size() + paths.size());
        for (auto& path : paths) { params.emplace_back(path.generic_string()); }
    }

    void AppendDispatch(const std::vector<std::string>& paths) {
        params.reserve(params.size() + paths.size());
        for (auto& path : paths) { params.emplace_back(path); }
    }
};

class Cmdopt {
public:
    Path in;
    Path out;
    Path err;

public:
};

inline bool run_cmd(const Cmd& cmd, Cmdopt opt = Cmdopt()) {
    if (cmd.empty()) {
        log(ERRO, "Could not run empty command");
        return false;
    }
    // log(CODE, cmd.GetCommandStr());
#ifdef _WIN32
    STARTUPINFOA        si = {sizeof(si)};
    PROCESS_INFORMATION pi;

    BOOL result = CreateProcessA(NULL, cmd.GetCommandStr().data(), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
    if (!result) {
        log(ERRO, "CreateProcess failed!");
        return false;
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;
#else
    pid_t cpid = fork();
    if (cpid < 0) {
        log(ERRO, "Coull not fork child process: %s", std::strerror(errno));
        return false;
    }
    if (cpid == 0) {
        // if ()
    }
#endif // _Win32
    return true;
}

namespace ToolChain {

enum class CompilerType {
    clang,
    gcc,
};

class Compiler {
public:
    Path path;

public:
    Compiler() = delete;
    Compiler(Path path) : path(path) {};

public:
    virtual bool generate_depfile(const Path& input, const Path output, const std::vector<std::string>& options, std::optional<Path> target) {
        std::vector<std::string> target_option = target ? std::vector<std::string>{"-MT", target->string()} : std::vector<std::string>{};

        Cmd cmd(path, "-MMD", "-MF", output, input, target_option, options);
        return run_cmd(cmd);
    }

    virtual bool compile_unit(const Path& input, const Path output, const std::vector<std::string> options) {
        Cmd cmd(path, "-c", input, "-o", output, options);
        return run_cmd(cmd);
    }

    virtual bool link_target(const Path& output, const std::vector<Path>& depfiles) {
        Cmd cmd(path, depfiles, "-o", output);
        return run_cmd(cmd);
    }

    virtual bool compile_and_gendep_unit(const Path& input, const Path obj, const Path& dep, const std::vector<std::string>& options) {
        Cmd cmd(path, "-c", input, "-o", obj, "-MMD", "-MF", dep, "-MT", obj, options);
        return run_cmd(cmd);
    }

private:
};

class ToolChain {
public:
    Dir base;
    Dir bin;

public:
    virtual Dir get_stdlib_dir() const = 0;
};

class Clang : public Compiler {
public:
    Clang() : Compiler("clang++") {};

    std::vector<std::string> compile_module_option(const Path& uints, const Path& targetdir = ".") {
        Path targetpath = targetdir / uints.filename().string();
        targetpath.replace_extension(".pcm");
        return {uints.string(), "--precompile", "-o", targetpath.string()};
    }

    std::vector<std::string> compile_stdmodule_option(const ToolChain& toolchain, const Path& targetdir = ".") {
        Path targetpath = targetdir / "std.pcm";
        return {toolchain.get_stdlib_dir().string(), "--precompile", "-o", targetpath.string()};
    }

private:
};
} // namespace ToolChain

inline std::error_code redirect_stream(const std::filesystem::path& path) {
    std::error_code ec;
#ifdef _WIN32

#else
    log(ERRO, "could not open file %s: %s", path.c_str(), std::strerror(errno));
#endif
    return ec;
}

namespace build {
class Graph {
public:
    std::vector<Path>                               units;
    std::unordered_map<size_t, std::vector<size_t>> dependences;

    // TODO: maybe string_view better
    std::unordered_map<Path, size_t> unit_map;

    void add_depinfo(const DepInfo& dep_info, const Translation_Unit& unit) {
        size_t unit_index = find_or_add(unit.path);

        dependences[unit_index].reserve(dep_info.depends.size());
        for (auto& dep : dep_info.depends) {
            size_t dep_index = find_or_add(dep);
            dependences[unit_index].push_back(dep_index);
        }
    }

    std::vector<Path> get_deps(const Translation_Unit& unit) {
        auto it = unit_map.find(unit.path);
        if (it == unit_map.end()) { return {}; }

        std::vector<Path> deps;

        auto& index = dependences[it->second];
        deps.reserve(index.size());

        for (auto i : index) {
            deps.push_back(units[i]);
        }
        return deps;
    }

private:
    size_t find_or_add(const Path& path) {
        auto [it, inserted] = unit_map.try_emplace(path, units.size());
        if (inserted) {
            units.push_back(path);
        }
        return it->second;
    }
};

inline bool check_rebuild(const Path& output, const std::vector<Path>& inputs) {
    using std::filesystem::last_write_time;
    BOOL bSuccess;

    if (!std::filesystem::exists(output)) { return true; }
    auto ouput_time = last_write_time(output);

    for (size_t i = 0; i < inputs.size(); ++i) {
        auto input_time = last_write_time(inputs[i]);

        if (input_time > ouput_time) {
            return true;
        }
    }

    return false;
}

inline Result<bool> update_self(int argc, char** argv, const Path& source_path, std::vector<Path> other_path = {}) {
    Path binary_path(argv[0]);
#ifdef _WIN32
    if (binary_path.extension() != ".exe") {
        binary_path.replace_extension(".exe");
    }
#else

#endif // _WIN32
    // bool need_rebuild = check_rebuild(binary_path, {});
    std::vector<Path> check_path = other_path;
    check_path.push_back(source_path);
    bool need_rebuild = check_rebuild(binary_path, check_path);

    if (!need_rebuild) {
        return false;
    }

    Cmd  compile_cmd;
    Path old_binary_path = binary_path;
    old_binary_path += ".old";
    std::filesystem::rename(binary_path, old_binary_path);
    compile_cmd.Append(predefine::current_compiler, "-std=c++23", "-o", binary_path, source_path);
    if (!run_cmd(compile_cmd)) {
        return Reason("compile failed");
    }
    // std::filesystem::remove(old_binary_path);
    Cmd exec_cmd(binary_path);
    exec_cmd.AppendRange(argc - 1, argv + 1);
    if (!run_cmd(exec_cmd)) {
        return Reason("exec cmd failed");
    }
    return true;
}

inline std::optional<DepInfo> parse_dep_file(const Path& dep_path) {
    Result<std::vector<char>> result = OS::ReadFile(dep_path);
    if (!result) {
        log(WARN, result.error());
        return std::nullopt;
    }

    // not good
    auto        data = result.value();
    std::string file;
    file.reserve(data.size());

    for (size_t i = 0; i < data.size(); ++i) {
        char c = data[i];

        if (c == '\\') {
            if (i + 1 < data.size() && data[i + 1] == '\n') {
                ++i;
                continue;
            }
            if (i + 2 < data.size() && data[i + 1] == '\r' && data[i + 2] == '\n') {
                i += 2;
                continue;
            }
        }

        file.push_back(c);
    }

    auto find_unescaped_colon = [](std::string_view s) {
        bool esc = false;
        for (size_t i = 0; i < s.size(); ++i) {
            if (esc) {
                esc = false;
                continue;
            }
            if (s[i] == '\\') {
                esc = true;
                continue;
            }
            if (s[i] == ':') return i;
        }
        return std::string_view::npos;
    };

    auto tokenize = [](std::string_view s) {
        auto                     is_ws = [](unsigned char c) { return std::isspace(c); };
        std::vector<std::string> out;
        size_t                   i = 0;
        while (i < s.size()) {
            while (i < s.size() && is_ws((unsigned char)s[i])) ++i;
            if (i >= s.size()) break;
            std::string tok;
            while (i < s.size() && !is_ws((unsigned char)s[i])) {
                if (s[i] == '\\' && i + 1 < s.size()) {
                    ++i;
                    tok.push_back(s[i++]);
                } else
                    tok.push_back(s[i++]);
            }
            out.push_back(std::move(tok));
        }
        return out;
    };

    size_t           colon = find_unescaped_colon(file);
    std::string_view lhs(file.data(), colon);
    std::string_view rhs(file.data() + colon + 1, file.size() - colon - 1);

    auto targets = tokenize(lhs);
    auto deps    = tokenize(rhs);
    return DepInfo{targets, deps};
}

// check success?
inline bool check_dep_file(const Translation_Unit& unit, const Path& dep_path, const Path& obj, Graph* graph = nullptr) {
    auto result = parse_dep_file(dep_path);
    if (!result) {
        return false;
    }
    DepInfo dep_info = result.value();
    if (graph) {
        graph->add_depinfo(dep_info, unit);
    }

    return check_rebuild(obj, dep_info.depends);
}

inline bool compile_translation_unit(ToolChain::Compiler& compiler, Translation_Unit& unit, const Dir& out_dir = "build", std::vector<std::string> options = {}, Graph* graph = nullptr) {
    Path obj = out_dir / unit.path.filename();
    obj.replace_extension(".o");
    Path dep = obj;
    dep.replace_extension(".d");
    unit.obj = obj;

    // TODO: generate_dependence maybe different by options,should add cache to diff

    bool need_rebuild = check_dep_file(unit, dep, obj, graph);
    if (!need_rebuild) {
        return true;
    }

    log(CODE, "%s need to rebuild", unit.path.string().c_str());
    bool result = false;
    std::filesystem::create_directories(out_dir);
    if (graph) {
        result = compiler.compile_and_gendep_unit(unit.path, obj, dep, options);
    } else {
        result = compiler.compile_unit(unit.path, obj, options);
    }

    // log(CODE, cmd.GetCommandStr());
    return result;
}

} // namespace build

class Target {
    enum class Type {
        exe,
        static_lib,
        dynamic_lib,
    };
    enum class Architecture {
        x86_64,
        aarch64,
        armv7,
        i686,
        arm64ec,
    };

    enum class CppVersion {
        cpp11,
        cpp14,
        cpp17,
        cpp20,
        cpp23,
        cpp26,
    };

public:
    Dir         out_dir = std::filesystem::current_path() / "build";
    Type        type    = Type::exe;
    std::string name    = "default target";

    CppVersion   version      = CppVersion::cpp23;
    Architecture architecture = Architecture::x86_64;

    std::set<std::string>         options;
    std::vector<Translation_Unit> units;

    build::Graph graph;

public:
    Target(const std::string& str) : name{str} {};

    void add_translation_units(const std::vector<Translation_Unit>& files) { units.append_range(files); };

    Path get_target_path(const Dir& out_dir) const {
        return out_dir / (name + ".exe");
    }

    std::vector<Path> obj_files() const {
        std::vector<Path> objs;
        objs.reserve(units.size());
        for (auto& i : units) {
            objs.push_back(i.obj);
        }
        return objs;
    }

    std::vector<std::string> get_options() const {
        return {options.begin(), options.end()};
    }

    void add_options(std::string_view str) {
        options.insert(std::string(str));
    }
};

class Project {
public:
    std::string name;
    Dir         root;
    Dir         build;

    std::vector<Target> targets;

public:
    Project() : name("default project") {};
    Project(const std::string& str) : name(str), root(std::filesystem::current_path()), build(root / "build") {};

    Target& add_target(Target&& target) noexcept {
        targets.emplace_back(std::move(target));
        return targets.back();
    };

    Target& operator[](std::string_view key) {
        for (auto& target : targets) {
            if (target.name == key) {
                return target;
            }
        }
        log(ERRO, "failed to get %s from project", key);
        std::abort();
    }

    const Target& operator[](std::string_view key) const noexcept {
        for (auto& target : targets) {
            if (target.name == key) {
                return target;
            }
        }
        log(ERRO, "failed to get %s from project", key);
        std::abort();
    }
};

inline bool build_target(ToolChain::Compiler& compiler, Target& target, const Dir& root = ".", const Dir& build_dir = "build") {
    for (auto& unit : target.units) {
        Dir relative = std::filesystem::relative(unit.path.parent_path(), root);
        Dir out_dir  = (build_dir / relative).lexically_normal();
        if (!build::compile_translation_unit(compiler, unit, out_dir, target.get_options(), &target.graph)) {
            log(ERRO, "compile %s failed", unit.path.generic_string().c_str());
            continue;
        }
    }

    // Path target_path = target.get_target_path(build_dir);
    // build::check_rebuild(target_path, )
    return compiler.link_target(target.get_target_path(build_dir), target.obj_files());
}

} // namespace csc
