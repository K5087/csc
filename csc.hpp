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
using string      = std::string;
using string_view = std::string_view;
using Dir         = std::filesystem::path;
using Path        = std::filesystem::path;

template <typename T>
using Result = std::expected<T, string>;
using Reason = std::unexpected<string>;

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
        return Reason("failed to open file! [\"" + path.string() + "\"]");
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
static string current_compiler = "clang++";
#elif define __GUN__
static string current_compiler = "gcc";
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

inline string get_ansi_color(Color color) {
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

inline string colorize_str(const std::string_view str, Color color) {
    if (OS::is_terminal()) {
        string result;
        result.reserve(str.size() + 16);
        result += "\x1b[";
        result += get_ansi_color(color);
        result += "m";
        result += str;
        result += "\x1b[0m";
        return result;
    } else {
        return string(str);
    }
}

inline void log_color(Level level, const string& fmt, va_list list) {
    switch (level) {
        case Level::code: std::fprintf(stderr, "%s", colorize_str("[code] ", Color::blue).c_str()); break;
        case Level::info: std::fprintf(stderr, "%s", colorize_str("[info] ", Color::green).c_str()); break;
        case Level::warn: std::fprintf(stderr, "%s", colorize_str("[warn] ", Color::yellow).c_str()); break;
        case Level::erro: std::fprintf(stderr, "%s", colorize_str("[erro] ", Color::red).c_str()); break;
    }
    std::vfprintf(stderr, fmt.c_str(), list);
    std::fprintf(stderr, "\n");
}

#define CODE log_impl::Level::code
#define INFO log_impl::Level::info
#define WARN log_impl::Level::warn
#define ERRO log_impl::Level::erro
} // namespace log_impl

inline void log(log_impl::Level level, string_view fmt, ...) {
    va_list list;
    va_start(list, fmt);
    log_color(level, string(fmt), list);
    va_end(list);
}

inline void logc(string_view fmt, ...) {
    va_list list;
    va_start(list, fmt);
    log_color(CODE, string(fmt), list);
    va_end(list);
}

inline void logi(string_view fmt, ...) {
    va_list list;
    va_start(list, fmt);
    log_color(INFO, string(fmt), list);
    va_end(list);
}

inline void logw(string_view fmt, ...) {
    va_list list;
    va_start(list, fmt);
    log_color(WARN, string(fmt), list);
    va_end(list);
}

inline void loge(string_view fmt, ...) {
    va_list list;
    va_start(list, fmt);
    log_color(ERRO, string(fmt), list);
    va_end(list);
}

inline bool is_outdated(const Path& output, const std::vector<Path>& inputs) {
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

struct DepInfo {
    std::vector<Path> targets;
    std::vector<Path> depends;
    DepInfo() = default;

    DepInfo(std::vector<string>& target, std::vector<std::string>& depend) :
    targets(target.begin(), target.end()),
    depends(depend.begin(), depend.end()) {
    };
};

inline std::optional<DepInfo> parse_dep_file(const Path& dep_path) {
    Result<std::vector<char>> result = OS::ReadFile(dep_path);
    if (!result) {
        log(WARN, result.error());
        return std::nullopt;
    }

    // not good
    auto   data = result.value();
    string file;
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

    auto find_unescaped_colon = [](string_view s) {
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
        return string_view::npos;
    };

    auto tokenize = [](string_view s) {
        auto                is_ws = [](unsigned char c) { return std::isspace(c); };
        std::vector<string> out;
        size_t              i = 0;
        while (i < s.size()) {
            while (i < s.size() && is_ws((unsigned char)s[i])) ++i;
            if (i >= s.size()) break;
            string tok;
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

    size_t      colon = find_unescaped_colon(file);
    string_view lhs(file.data(), colon);
    string_view rhs(file.data() + colon + 1, file.size() - colon - 1);

    auto targets = tokenize(lhs);
    auto deps    = tokenize(rhs);
    return DepInfo{targets, deps};
}

class Unit {
public:
    Path path;
    Path obj;

    Unit(Path path) : path(path) {};

    constexpr virtual bool is_module() const noexcept {
        return false;
    }
};

class Module : public Unit {
public:
    constexpr virtual bool is_module() const noexcept {
        return false;
    }
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

    string GetCommandStr() const {
        string Command;
        for (size_t i = 0; i < params.size(); i++) {
            string_view arg = params[i];
            if (i > 0) Command.append(" ");
            if (!arg.empty() && string_view::npos == arg.find_first_of(" \t\n\v\"")) {
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
    std::vector<string> params;

private:
    void AppendDispatch(const char* s) { params.emplace_back(s); }

    void AppendDispatch(const string& s) { params.emplace_back(s); }

    void AppendDispatch(const string&& s) { params.emplace_back(std::move(s)); }

    void AppendDispatch(string_view s) { params.emplace_back(s); }

    void AppendDispatch(const Path& p) {
        params.emplace_back(p.generic_string());
    }

    void AppendDispatch(const std::vector<Path>& paths) {
        params.reserve(params.size() + paths.size());
        for (auto& path : paths) { params.emplace_back(path.generic_string()); }
    }

    void AppendDispatch(const std::vector<string>& paths) {
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
        log(ERRO, "Could not run empty command!");
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
    DWORD ec;
    GetExitCodeProcess(pi.hProcess, &ec);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return ec == 0;
#else
    pid_t cpid = fork();
    if (cpid < 0) {
        log(ERRO, "Coull not fork child process: %s!", std::strerror(errno));
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
    Path exe;

public:
    Compiler() = delete;
    Compiler(Path path) : exe(path) {};

public:
    virtual bool link_target(const Path& output, const std::vector<Path>& depfiles) {
        Cmd cmd(exe, depfiles, "-o", output);
        return run_cmd(cmd);
    }

    virtual std::vector<std::string> compile_flag(const Path& unit)                   = 0;
    virtual std::vector<std::string> out_flag(const Path& unit)                       = 0;
    virtual std::vector<std::string> dep_flag(const Path& unit, const Path& obj = "") = 0;

    virtual Cmd get_compile_unit_cmd(const Path& input, const Path& output, const std::vector<string>& options) {
        return {exe, "-c", input, "-o", output, options};
    }

    virtual Cmd get_link_target_cmd(const Path& output, const std::vector<Path>& depfiles) {
        return {exe, depfiles, "-o", output};
    }

    virtual Cmd get_compile_and_gendep_unit_cmd(const Path& input, const Path& obj, const Path& dep, const std::vector<string>& options) {
        return {exe, "-c", input, "-o", obj, "-MMD", "-MF", dep, "-MT", obj, options};
    }

    virtual Cmd get_compile_module_cmd(const Path& input, const Path& output, const std::vector<string>& options) {
        return {exe, "--precompile", "-o", output};
    }
};

class GNU_Compiler : public Compiler {
public:
    // GNU_Compiler(const Path& path) : Compiler(path) {};
    using Compiler::Compiler;

    virtual std::vector<std::string> compile_flag(const Path& unit) {
        return {"-c", unit.generic_string()};
    }

    virtual std::vector<std::string> out_flag(const Path& obj) {
        return {"-o", obj.generic_string()};
    }

    virtual std::vector<std::string> dep_flag(const Path& dep, const Path& obj = "") {
        std::vector<std::string> flags = {"-MMD", "-MF", dep.generic_string()};
        if (!obj.empty()) {
            flags.push_back("-MT");
            flags.push_back(obj.generic_string());
        }

        return flags;
    }
};

class Clang : public GNU_Compiler {
public:
    using GNU_Compiler::GNU_Compiler;
    Clang() : GNU_Compiler("clang++") {};

    std::vector<string> compile_module_option(const Path& uints, const Path& targetdir = ".") {
        Path targetpath = targetdir / uints.filename().string();
        targetpath.replace_extension(".pcm");
        return {uints.generic_string(), "--precompile", "-o", targetpath.string()};
    }

private:
};

inline std::shared_ptr<Compiler> find_compiler(const Dir& dir) {
    Path exe = dir / "bin" / "clang++";
    if (std::filesystem::exists(exe)) {
        return std::make_shared<Clang>(exe);
    }
    throw std::runtime_error("undetected compiler.");
}

class ToolChain {
public:
    Dir base;

public:
    ToolChain() = delete;

    ToolChain(const Dir& path) : base(path) {};

    virtual Dir  get_stdlib_dir() const;
    virtual bool scan_module_dep(const Path& input, const std::vector<string>& compile_command);

private:
};

class LLVM_MinGW : public ToolChain {
public:
    using ToolChain::ToolChain;

    virtual Dir get_stdlib_dir() const override {
        return base / "share" / "libc++" / "v1";
    }

    virtual bool scan_module_dep(const Path& input, const std::vector<string>& compile_command) override {
        Cmd cmd(base / "bin" / "clang-scan-deps", "--format=p1689", "--", compile_command);
        return run_cmd(cmd);
    }

private:
};

} // namespace ToolChain

// TODO: implenment it
inline ToolChain::ToolChain find_ToolChain(const Dir& dir) {
    Path path = dir / "bin" / "clang++.exe";
    if (std::filesystem::exists(path)) {
        return ToolChain::LLVM_MinGW(dir);
    }

    throw std::runtime_error("unknown toolchain");
}

inline std::error_code redirect_stream(const std::filesystem::path& path) {
    std::error_code ec;
#ifdef _WIN32

#else
    log(ERRO, "could not open file %s: %s!", path.c_str(), std::strerror(errno));
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

    void add_depinfo(const DepInfo& dep_info, const Unit& unit) {
        size_t unit_index = find_or_add(unit.path);

        dependences[unit_index].reserve(dep_info.depends.size());
        for (auto& dep : dep_info.depends) {
            size_t dep_index = find_or_add(dep);
            dependences[unit_index].push_back(dep_index);
        }
    }

    std::vector<Path> get_deps(const Unit& unit) {
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

inline Result<bool> update_self(int argc, char** argv, const Path& source_path, const std::vector<Path>& other_path = {}) {
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
    bool need_rebuild = is_outdated(binary_path, check_path);

    if (!need_rebuild) {
        return false;
    }

    logi("build program start update");
    Cmd  compile_cmd;
    Path old_binary_path = binary_path;
    old_binary_path += ".old";
    std::filesystem::rename(binary_path, old_binary_path);
    compile_cmd.Append(predefine::current_compiler, "-std=c++23", "-o", binary_path, source_path);
    if (!run_cmd(compile_cmd)) {
        return Reason("compile build script failed");
    }
    logi("update build program success");
    // std::filesystem::remove(old_binary_path);
    Cmd exec_cmd(binary_path);
    exec_cmd.AppendRange(argc - 1, argv + 1);
    if (!run_cmd(exec_cmd)) {
        return Reason("exec new build program failed");
    }
    return true;
}

// check success?
inline bool check_dep_file(const Unit& unit, const Path& dep_path, const Path& obj, Graph* graph = nullptr) {
    if (!std::filesystem::exists(dep_path)) { return false; }
    auto result = parse_dep_file(dep_path);
    if (!result) {
        return false;
    }
    DepInfo dep_info = result.value();
    if (graph) {
        graph->add_depinfo(dep_info, unit);
    }

    return !is_outdated(obj, dep_info.depends);
}

inline bool compile_translation_unit(ToolChain::Compiler& compiler, Unit& unit, const Dir& out_dir = "build", std::vector<string> options = {}, Graph* graph = nullptr) {
    Path out_name = out_dir / unit.path.filename();
    Path obj      = out_dir / unit.path.filename();
    obj.replace_extension(".o");
    Path dep = obj;
    dep.replace_extension(".d");
    unit.obj = obj;

    // TODO: generate_dependence maybe different by options,should add cache to diff

    bool need_rebuild = !std::filesystem::exists(obj) || !check_dep_file(unit, dep, obj, graph);
    if (!need_rebuild) {
        return true;
    }

    log(INFO, "%s need to rebuild.", unit.path.string().c_str());
    bool result = false;
    std::filesystem::create_directories(out_dir);
    Cmd cmd;

    if (unit.is_module()) {
        cmd = compiler.get_compile_module_cmd(unit.path, obj, options);
    } else {
        if (graph) {
            cmd = compiler.get_compile_and_gendep_unit_cmd(unit.path, obj, dep, options);
        } else {
            cmd = compiler.get_compile_unit_cmd(unit.path, obj, options);
        }
    }

    // log(CODE, cmd.GetCommandStr());
    return run_cmd(cmd);
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
    Dir    root  = ".";
    Dir    build = "build";
    Type   type  = Type::exe;
    string name  = "default_target";

    CppVersion   version      = CppVersion::cpp23;
    Architecture architecture = Architecture::x86_64;

    std::set<string>  options;
    std::vector<Unit> units;

    build::Graph graph;

public:
    Target(const string& str) :
    name{str} {};

    void add_translation_units(const std::vector<Unit>& files) { units.append_range(files); };

    Path get_target_path(const Dir& out_dir = "") const {
        if (out_dir.empty()) {
            return build / (name + ".exe");
        }
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

    std::vector<string> get_options() const {
        return {options.begin(), options.end()};
    }

    void add_options(string_view str) {
        options.insert(string(str));
    }
};

class Project {
public:
    string name;
    Dir    root;
    Dir    build;

    std::vector<Target> targets;

public:
    Project() :
    name("default project") {};
    Project(const string& str) :
    name(str),
    root(std::filesystem::current_path()),
    build(root / "build") {};

    Target& add_target(Target&& target) noexcept {
        targets.emplace_back(std::move(target));
        return targets.back();
    };

    Target& operator[](string_view key) {
        for (auto& target : targets) {
            if (target.name == key) {
                return target;
            }
        }
        log(ERRO, "failed to get %s from project.", key);
        std::abort();
    }

    const Target& operator[](string_view key) const noexcept {
        for (auto& target : targets) {
            if (target.name == key) {
                return target;
            }
        }
        log(ERRO, "failed to get %s from project.", key);
        std::abort();
    }
};

inline bool build_target(ToolChain::Compiler& compiler, Target& target) {
    for (auto& unit : target.units) {
        Dir relative = std::filesystem::relative(unit.path.parent_path(), target.root);
        Dir out_dir  = (target.build / relative).lexically_normal();

        if (!build::compile_translation_unit(compiler, unit, out_dir, target.get_options(), &target.graph)) {
            log(ERRO, "compile %s failed.", unit.path.generic_string().c_str());
            // continue;
            return false;
        }
    }

    return compiler.link_target(target.get_target_path(), target.obj_files());
}

inline void update_self(int argc, char** argv, const Path& source, const std::vector<Path>& others = {}) {
    auto result = build::update_self(argc, argv, source, others);
    if (!result) {
        log(ERRO, result.error());
    } else {
        // update success,exit old build
        if (result.value()) std::exit(0);
    }
}
} // namespace csc
