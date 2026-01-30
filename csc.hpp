#include <cstdarg>
#include <expected>
#include <filesystem>
#include <string>
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
using Path = std::filesystem::path;

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

namespace Error_Handle {

template <typename T>
using Result = std::expected<T, std::string>;
using Reason = std::unexpected<std::string>;

} // namespace Error_Handle

class Translation_Unit {
public:
    Path path;

    Translation_Unit(Path path) : path(path) {};
};

namespace ToolChain {

enum class CompilerType {
    clang,
    gcc,
};

template <CompilerType T>
class Compiler {
public:
    Path path;

public:
    Compiler() = delete;
    Compiler(Path path) : path(path) {};
};

class ToolChain {
    using dir = std::filesystem::path;

public:
    dir base;
    dir bin;

    dir get_stdlib_dir() const {
        return base / "share/libc++/v1/std.cppm";
    }
};

class Clang : public Compiler<CompilerType::clang> {
public:
    std::vector<std::string> compile_module_option(const Translation_Unit& uints, const Path& targetpath = ".") {
        return {uints.path.string(), "--precompile", "-o", (targetpath / uints.path.filename()).string()};
    }

    std::vector<std::string> compile_stdmodule_option(const ToolChain& toolchain, const Path& targetpath = ".") {
        return {toolchain.get_stdlib_dir().string(), "--precompile", "-o", (targetpath / "std.pcm").string()};
    }

private:
};
} // namespace ToolChain

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

public:
    Type                          type         = Type::exe;
    std::string                   name         = "default target";
    Architecture                  architecture = Architecture::x86_64;
    std::vector<Translation_Unit> translations;
};

class Project {
    using dir = std::filesystem::path;

public:
    std::string name  = "default project";
    dir         build = std::filesystem::current_path() / "build";

    std::vector<Target> targets;

public:
    Project(const std::string& str) : name(str) {};
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
        params.emplace_back(p.string());
    }

    void AppendDispatch(const std::vector<Path>& paths) {
        params.reserve(params.size() + paths.size());
        for (auto& path : paths) { params.emplace_back(path.string()); }
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

inline std::error_code redirect_stream(const std::filesystem::path& path) {
    std::error_code ec;
#ifdef _WIN32

#else
    log(ERRO, "could not open file %s: %s", path.c_str(), std::strerror(errno));
#endif
    return ec;
}

inline bool run_cmd(const Cmd& cmd, Cmdopt opt = Cmdopt()) {
    if (cmd.empty()) {
        log(ERRO, "Could not run empty command");
        return false;
    }
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

namespace build {
using std::filesystem::path;
using Reason = csc::Error_Handle::Reason;

template <class T>
using Result = csc::Error_Handle::Result<T>;

inline bool check_rebuild(const path& output, const std::vector<path>& inputs) {
    using std::filesystem::last_write_time;
    BOOL bSuccess;

    auto ouput_time = last_write_time(output);

    for (size_t i = 0; i < inputs.size(); ++i) {
        auto input_time = last_write_time(inputs[i]);

        if (input_time > ouput_time) {
            return true;
        }
    }

    return false;
}

inline Result<bool> update_self(int argc, char** argv, const path& source_path) {
    using namespace Error_Handle;

    path binary_path(argv[0]);
#ifdef _WIN32
    if (binary_path.extension() != ".exe") {
        binary_path.replace_extension(".exe");
    }
#else

#endif // _WIN32
    // bool need_rebuild = check_rebuild(binary_path, {});
    bool need_rebuild = check_rebuild(binary_path, {source_path});

    if (!need_rebuild) {
        return false;
    }

    Cmd  compile_cmd;
    path old_binary_path = binary_path;
    old_binary_path += ".old";
    std::filesystem::rename(binary_path, old_binary_path);
    compile_cmd.Append(predefine::current_compiler, "-std=c++23", "-o", binary_path, source_path);
    if (!run_cmd(compile_cmd)) {
        return Reason("compile failed");
    }
    std::filesystem::remove(old_binary_path);
    Cmd exec_cmd(binary_path);
    exec_cmd.AppendRange(argc - 1, argv + 1);
    if (!run_cmd(exec_cmd)) {
        return Reason("exec cmd failed");
    }
    return true;
}

template <ToolChain::CompilerType T>
inline bool compile_translation_unit(ToolChain::Compiler<T>& compiler, std::vector<std::string> options = {}) {
    Cmd cmd(compiler.path, options);
    return run_cmd(cmd);
}
} // namespace build
} // namespace csc
