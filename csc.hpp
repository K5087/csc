#include <cstdarg>
#include <cstring>
#include <expected>
#include <filesystem>
#include <optional>
#include <semaphore>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <errno.h>
    #include <sys/wait.h>
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

namespace predefine {
#if defined __clang__
static string current_compiler = "clang++";
#elif define __GUN__
static string current_compiler = "gcc";
#endif // __clang__

} // namespace predefine

namespace log {
enum Level {
    code,
    info,
    warn,
    erro,
};

namespace impl {
inline void log(Level level, const string& fmt, va_list list) {
    switch (level) {
        case Level::code: std::fprintf(stderr, "[code] "); break;
        case Level::info: std::fprintf(stderr, "[info] "); break;
        case Level::warn: std::fprintf(stderr, "[warn] "); break;
        case Level::erro: std::fprintf(stderr, "[erro] "); break;
    }
    std::vfprintf(stderr, fmt.c_str(), list);
    std::fprintf(stderr, "\n");
}
} // namespace impl

#define CODE log::Level::code
#define INFO log::Level::info
#define WARN log::Level::warn
#define ERRO log::Level::erro

inline void log(log::Level level, string_view fmt, ...) {
    va_list list;
    va_start(list, fmt);
    log::impl::log(level, string(fmt), list);
    va_end(list);
}
} // namespace log

inline void logc(string_view fmt, ...) {
    va_list list;
    va_start(list, fmt);
    log::impl::log(CODE, string(fmt), list);
    va_end(list);
}

inline void logi(string_view fmt, ...) {
    va_list list;
    va_start(list, fmt);
    log::impl::log(INFO, string(fmt), list);
    va_end(list);
}

inline void logw(string_view fmt, ...) {
    va_list list;
    va_start(list, fmt);
    log::impl::log(WARN, string(fmt), list);
    va_end(list);
}

inline void loge(string_view fmt, ...) {
    va_list list;
    va_start(list, fmt);
    log::impl::log(ERRO, string(fmt), list);
    va_end(list);
}

inline bool is_outdated(const Path& output, const std::vector<Path>& inputs) {
    using std::filesystem::last_write_time;

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

    std::vector<char*> GetCommandList() const {
        std::vector<char*> argv;
        argv.reserve(params.size() + 1);
        for (auto& s : params) {
            argv.push_back(const_cast<char*>(s.c_str()));
        }
        argv.push_back(nullptr);
        return argv;
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
    bool wait_return = true;

public:
};

namespace cmd_impl {
#ifdef _WIN32
inline thread_local unsigned long error;
using Proc = void*;
#else
inline thread_local int error;
using Proc = int;
#endif // _WIN32

static std::vector<Proc>         procs;
static std::counting_semaphore<> sem(std::thread::hardware_concurrency() + 1);

// create a process to run command
inline Proc create_proc(const Cmd& cmd, const Cmdopt& opt) {
    auto in = opt.in;
#ifdef _WIN32
    STARTUPINFO         si = {};
    PROCESS_INFORMATION pi;

    si.cb       = sizeof(si);
    BOOL result = CreateProcess(NULL, cmd.GetCommandStr().data(), NULL, NULL, TRUE,
                                0, NULL, NULL, &si, &pi);
    if (!result) {
        error = GetLastError();
        return Proc(nullptr);
    }
    CloseHandle(pi.hThread);
    return Proc(pi.hProcess);
#else
    pid_t cpid = fork();
    if (cpid < 0) {
        perror("fork failed");
        return Proc(0);
    }
    if (cpid == 0) {
        auto argv = cmd.GetCommandList();
        execvp(argv[0], argv.data());
        perror("execvp");
        _exit(127);
    }
    return cpid;
#endif
}

// check process is running
inline bool is_running(Proc proc) noexcept {
#ifdef _WIN32
    if (!proc) {
        return false; // is must need?
    }
    DWORD result = WaitForSingleObject(proc, 0);
    if (result == WAIT_TIMEOUT) {
        return true;
    }

    return false;
#else
    int   status = 0;
    pid_t result = waitpid(proc, &status, WNOHANG | WUNTRACED | WCONTINUED);
    if (result == 0)
        return true;
    else if (result < 0)
        return false;
    if (WIFEXITED(status) || WIFSIGNALED(status))
        return false;
    else if (WIFSTOPPED(status) || WIFCONTINUED(status))
        return true;
    return true;
#endif
}

// wait until process return
inline std::optional<int> wait_proc(Proc proc) noexcept {
#ifdef _WIN32
    DWORD result = WaitForSingleObject(proc, INFINITE);
    if (result == WAIT_FAILED) {
        error = GetLastError();
        return std::nullopt;
    }
    DWORD exit_status;
    if (!GetExitCodeProcess(proc, &exit_status)) {
        error = GetLastError();
        return std::nullopt;
    }

    CloseHandle(proc);
    return exit_status;
#else
    int status = 0;
    if (waitpid(proc, &status, 0) < 0) {
        error = errno;
        return std::nullopt;
    }
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return std::nullopt;
#endif
}

inline bool check_procs() {
    bool result = false;
    for (auto iter = procs.begin(); iter != procs.end();) {
        if (is_running(*iter)) {
            iter = procs.erase(iter);
            sem.release();
            result = true;
        } else {
            iter++;
        }
    }
    return result;
}

// run an command
inline std::optional<int> run_cmd(const Cmd& cmd, Cmdopt opt = {}) noexcept {
    if (cmd.empty()) {
        loge("Could not run empty command");
        return std::nullopt;
    }

    check_procs();

    while (!sem.try_acquire()) {
        logi("cmd process num too much,wait someone quit");
        std::this_thread::sleep_for(std::chrono::seconds(2));
        check_procs();
    }

    Proc proc = create_proc(cmd, opt);

    if (!proc) {
        sem.release();
        return std::nullopt;
    }

    if (opt.wait_return) {
        auto ret = wait_proc(proc);
        sem.release();
        return ret;
    } else {
        procs.push_back(proc);
    }

    return 0;
}
} // namespace cmd_impl

inline string GetErrorMessage() {
#ifdef _WIN32
    static char buf[4 * 1024] = {0};

    DWORD len = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, cmd_impl::error, LANG_USER_DEFAULT, buf,
                               4 * 1024, NULL);

    if (len == 0) {
        if (cmd_impl::error != ERROR_MR_MID_NOT_FOUND) {
            if (sprintf(buf, "Could not get error message for 0x%lX", cmd_impl::error) > 0) {
                return (char*)&buf;
            } else {
                return "";
            }
        } else {
            if (sprintf(buf, "Invalid Windows Error code (0x%lX)", cmd_impl::error) > 0) {
                return (char*)&buf;
            } else {
                return "";
            }
        }
    }

    while (len > 1 && isspace(buf[len - 1])) {
        buf[--len] = '\0';
    }

    return buf;
#else
    return strerror(cmd_impl::error);
#endif // _WIN32
}

inline bool run_cmd(const Cmd& cmd, Cmdopt opt = {}) noexcept {
    auto result = cmd_impl::run_cmd(cmd, opt);
    if (!result) {
        loge("run cmd failed: %s(%s)", GetErrorMessage().c_str(), cmd.GetCommandStr().c_str());
        return false;
    }
    return true;
}

namespace build {

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
#ifdef _WIN32
#else
    std::filesystem::remove(old_binary_path);
#endif
    Cmd exec_cmd(binary_path);
    exec_cmd.AppendRange(argc - 1, argv + 1);
    if (!run_cmd(exec_cmd)) {
        return Reason("exec new build program failed");
    }
    return true;
}

} // namespace build

inline void update_self(int argc, char** argv, const Path& source, const std::vector<Path>& others = {}) {
    auto result = build::update_self(argc, argv, source, others);
    if (!result) {
        loge(result.error());
    } else {
        // update success,exit old build
        if (result.value()) std::exit(0);
    }
}

#define CMD(...) run_cmd(Cmd(__VA_ARGS__))
} // namespace csc
