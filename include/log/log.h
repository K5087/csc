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

void log_level(Level level, const char* fmt, va_list args);

} // namespace logger

/**
 * @brief log something
 *
 * @param fmt
 */
void log(const char* fmt, ...);

/**
 * @brief log level code
 *
 * @param fmt
 */
void logc(const char* fmt, ...);

/**
 * @brief log level info
 *
 * @param fmt
 */
void logi(const char* fmt, ...);

/**
 * @brief log level warning
 *
 * @param fmt
 */
void logw(const char* fmt, ...);

/**
 * @brief log level error
 *
 * @param fmt
 */
void loge(const char* fmt, ...);
