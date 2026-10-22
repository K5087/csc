#include <log/log.h>

#include <cstdarg>
#include <cstdio>

namespace logger {

const char* tostring(Level level) {
    switch (level) {
        case Level::code: return "code";
        case Level::info: return "info";
        case Level::warn: return "warn";
        case Level::erro: return "erro";
        default: return "unkn";
    }
}

void log_level(Level level, const char* fmt, va_list args) {
    std::fprintf(stderr, "[%s] ", tostring(level));
    std::vfprintf(stderr, fmt, args);
    std::putc('\n', stderr);
}

} // namespace logger

void log(const char* fmt, ...) {
    va_list list;
    va_start(list, fmt);
    std::vfprintf(stderr, fmt, list);
    va_end(list);
    std::putc('\n', stderr);
}

void logc(const char* fmt, ...) {
    va_list list;
    va_start(list, fmt);
    logger::log_level(logger::Level::code, fmt, list);
    va_end(list);
}

void logi(const char* fmt, ...) {
    va_list list;
    va_start(list, fmt);
    logger::log_level(logger::Level::info, fmt, list);
    va_end(list);
}

void logw(const char* fmt, ...) {
    va_list list;
    va_start(list, fmt);
    logger::log_level(logger::Level::warn, fmt, list);
    va_end(list);
}

void loge(const char* fmt, ...) {
    va_list list;
    va_start(list, fmt);
    logger::log_level(logger::Level::erro, fmt, list);
    va_end(list);
}
