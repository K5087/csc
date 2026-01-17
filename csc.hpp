#include <cstdarg>
#include <filesystem>
#include <string>
#include <vector>

#include <cstring>
#include <system_error>

#ifdef _Win32
    #include <windows.h>
#else
    #include <errno.h>
    #include <unistd.h>
#endif // _Win32

using std::filesystem::path;

namespace csc {
namespace log {
enum Level {
    code,
    info,
    warn,
    erro,
};

inline void log_impl(Level level, const std::string& fmt, va_list list) {
    switch (level) {
        case Level::code: std::fprintf(stderr, "[code]"); break;
        case Level::info: std::fprintf(stderr, "[info]"); break;
        case Level::warn: std::fprintf(stderr, "[warn]"); break;
        case Level::erro: std::fprintf(stderr, "[erro]"); break;
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

class Cmd {
public:
    template <typename... T>
    void Append(T&&... t) { (AppendDispatch(std::forward<T>(t)), ...); }

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
        for (auto& path : paths) { params.emplace_back(path); }
    }
};

class Cmdopt {
public:
    path stdin;
    path stdout;
    path stderr;

public:
};

inline std::error_code redirect_stream(const path& path) {
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
#ifdef _Win32

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
} // namespace csc
