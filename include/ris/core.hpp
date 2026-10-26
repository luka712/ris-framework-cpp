#pragma once

#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>

#include "ris_export.h"

namespace ris {

inline constexpr int version_major = 0;
inline constexpr int version_minor = 1;
inline constexpr int version_patch = 0;

/// "major.minor.patch".
RIS_API const char* version_string() noexcept;

// The C++ API throws standard exceptions: std::invalid_argument (bad arguments, wrong call order),
// std::out_of_range (an index or range outside its object), std::domain_error (a format / usage / feature the
// backend does not support), std::logic_error (no WindowManager yet), std::ios_base::failure (a file could not be
// read or written) and std::runtime_error (SDL / EGL / OpenGL ES / shader / KTX2 failures). The C API converts
// them to a `ris_result` and `ris_get_last_error()`.

enum class LogLevel : int32_t { Trace = 0, Debug = 1, Info = 2, Warn = 3, Error = 4, Off = 5 };

/// Called for every message at or above the log level. May be called from any thread.
/// The default sink writes to stderr. Pass an empty function to restore it.
RIS_API void set_log_callback(std::function<void(LogLevel level, const char* message)> callback);
RIS_API void set_log_level(LogLevel level) noexcept;

/// Monotonic seconds since the first call to any time function.
RIS_API double time_seconds() noexcept;
RIS_API void sleep_ms(uint32_t milliseconds) noexcept;

} // namespace ris
