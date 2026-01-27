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

namespace OS {
inline bool is_terminal() {
#ifdef _WIN32
    return GetFileType(GetStdHandle(STD_OUTPUT_HANDLE)) == FILE_TYPE_CHAR;
#else
    return isatty(fileno(stderr));
#endif // _WIN32
}
} // namespace OS

namespace Compiler {
#if defined __clang__
static std::string current_compiler = "clang++";
#elif define __GUN__
static std::string current_compiler = "gcc";
#endif // __clang__

} // namespace Compiler

namespace log {
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

inline void log_impl(Level level, const std::string& fmt, va_list list) {
    switch (level) {
        case Level::code: std::fprintf(stderr, "%s", colorize_str("[code]", Color::blue).c_str()); break;
        case Level::info: std::fprintf(stderr, "%s", colorize_str("[info]", Color::green).c_str()); break;
        case Level::warn: std::fprintf(stderr, "%s", colorize_str("[warn]", Color::yellow).c_str()); break;
        case Level::erro: std::fprintf(stderr, "%s", colorize_str("[erro]", Color::red).c_str()); break;
    }
    std::vfprintf(stderr, fmt.c_str(), list);
    std::fprintf(stderr, "\n");
}

#define CSC_CODE log::Level::code
#define CSC_INFO log::Level::info
#define CSC_WARN log::Level::warn
#define CSC_ERRO log::Level::erro
} // namespace log

inline void csc_log(log::Level level, std::string_view fmt, ...) {
    va_list list;
    va_start(list, fmt);
    log_impl(level, std::string(fmt), list);
    va_end(list);
}

namespace Error_Handle {

enum class build_error {
    ok,
    exec,
};

class ErrorInfo {
public:
    std::string message;

public:
    ErrorInfo(const std::string_view str) { message = str; }
};

template <typename T>
using Result = std::expected<T, ErrorInfo>;
using Reason = std::unexpected<ErrorInfo>;

} // namespace Error_Handle

class Cmd {
    using path = std::filesystem::path;

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
    template <class T>
    void AppendDispatch(T&& t) {
        AppendOne(std::forward<T>(t));
    }

    void AppendOne(const char* s) { params.emplace_back(s); }

    void AppendOne(const std::string& s) { params.emplace_back(s); }

    void AppendOne(std::string_view s) { params.emplace_back(s); }

    void AppendOne(const path& p) {
        params.emplace_back(p.string());
    }

    void AppendRange(const std::vector<path>& paths) {
        params.reserve(params.size() + paths.size());
        for (auto& path : paths) { params.emplace_back(path.string()); }
    }
};

class Cmdopt {
    using path = std::filesystem::path;

public:
    path in;
    path out;
    path err;

public:
};

inline std::error_code redirect_stream(const std::filesystem::path& path) {
    std::error_code ec;
#ifdef _WIN32

#else
    csc_log(CSC_ERRO, "could not open file %s: %s", path.c_str(), std::strerror(errno));
#endif
    return ec;
}

inline bool run_cmd(const Cmd& cmd, Cmdopt opt = Cmdopt()) {
    if (cmd.empty()) {
        csc_log(CSC_ERRO, "Could not run empty command");
        return false;
    }
#ifdef _WIN32
    STARTUPINFOA        si = {sizeof(si)};
    PROCESS_INFORMATION pi;

    BOOL result = CreateProcessA(NULL, cmd.GetCommandStr().data(), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
    if (!result) {
        csc_log(CSC_ERRO, "CreateProcess failed!");
        return false;
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;
#else
    pid_t cpid = fork();
    if (cpid < 0) {
        csc_log(CSC_ERRO, "Coull not fork child process: %s", std::strerror(errno));
        return false;
    }
    if (cpid == 0) {
        // if ()
    }
#endif // _Win32
    return true;
}

inline std::error_code mkdir_if_noexist(const std::filesystem::path& path) {
    std::error_code ec;
    std::filesystem::create_directories(path, ec);
    return ec;
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

inline Result<bool> register_rebuild_self(int argc, char** argv, const path& source_path) {
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
    compile_cmd.Append(Compiler::current_compiler, "-std=c++23", "-o", binary_path, source_path);
    if (!run_cmd(compile_cmd)) {
        return Reason("compile failed");
    }
    Cmd exec_cmd(binary_path);
    exec_cmd.AppendRange(argc - 1, argv);
    if (!run_cmd(exec_cmd)) {
        return Reason("exec cmd failed");
    }
    return true;
}

} // namespace build
} // namespace csc
