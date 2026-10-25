#pragma once

// Internal declarations shared by the framework's translation units. Not installed.

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <string>

#include <GLES3/gl3.h>

#include "ris/core.hpp"

namespace ris::detail {

void log(LogLevel level, const char* fmt, ...)
#if defined(__GNUC__)
    __attribute__((format(printf, 2, 3)))
#endif
    ;

#define RIS_LOG_T(...) ::ris::detail::log(::ris::LogLevel::Trace, __VA_ARGS__)
#define RIS_LOG_D(...) ::ris::detail::log(::ris::LogLevel::Debug, __VA_ARGS__)
#define RIS_LOG_I(...) ::ris::detail::log(::ris::LogLevel::Info, __VA_ARGS__)
#define RIS_LOG_W(...) ::ris::detail::log(::ris::LogLevel::Warn, __VA_ARGS__)
#define RIS_LOG_E(...) ::ris::detail::log(::ris::LogLevel::Error, __VA_ARGS__)

/// Throws std::logic_error unless a ris::Window (and thus a GL context) exists.
void require_context();
bool has_context() noexcept;
/// Called by Window.
void set_context_alive(bool alive) noexcept;

/// Throws std::runtime_error if glGetError reports an error.
void check_gl(const char* what);

} // namespace ris::detail
