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

namespace impl {
void log(Level level, const char* fmt, va_list args) {
    std::fprintf(stderr, "[%s] ", tostring(level));
    std::vfprintf(stderr, fmt, args);
    std::putc('\n', stderr);
}
} // namespace impl

void log(Level level, const char* fmt, ...) {
    va_list list;
    va_start(list, fmt);
    impl::log(level, fmt, list);
    va_end(list);
}

} // namespace logger

void logc(const char* fmt, ...) {
    va_list list;
    va_start(list, fmt);
    logger::impl::log(logger::Level::code, fmt, list);
    va_end(list);
}

void logi(const char* fmt, ...) {
    va_list list;
    va_start(list, fmt);
    logger::impl::log(logger::Level::info, fmt, list);
    va_end(list);
}

void logw(const char* fmt, ...) {
    va_list list;
    va_start(list, fmt);
    logger::impl::log(logger::Level::warn, fmt, list);
    va_end(list);
}

void loge(const char* fmt, ...) {
    va_list list;
    va_start(list, fmt);
    logger::impl::log(logger::Level::erro, fmt, list);
    va_end(list);
}
