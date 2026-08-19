#pragma once

typedef __builtin_va_list va_list;

namespace logger {

enum class Level {
    code,
    info,
    warn,
    erro,
};

const char* tostring(Level level);

namespace impl {

void log(Level level, const char* fmt, va_list args);

} // namespace impl

void log(Level level, const char* fmt, ...);

} // namespace logger

void log(const char* fmt, ...);

void logc(const char* fmt, ...);

void logi(const char* fmt, ...);

void logw(const char* fmt, ...);

void loge(const char* fmt, ...);
