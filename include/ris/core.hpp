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

/// Error categories. The values match the C API's `ris_result`.
enum class Result : int32_t {
    Ok = 0,
    InvalidArgument = -1,
    NotInitialized = -2,
    Platform = -3,    ///< SDL / OS failure
    Graphics = -4,    ///< EGL / ANGLE / OpenGL ES failure
    Io = -5,          ///< a file could not be read
    Unsupported = -6,
    OutOfMemory = -7,
    Compile = -8,     ///< shader compile / link failure
    Ktx = -9          ///< KTX2 loading / transcoding failure
};

/// Thrown by every throwing function of the C++ API. The C API converts it to a `ris_result`
/// and `ris_get_last_error()`.
class RIS_API Error : public std::runtime_error {
public:
    Error(Result code, const std::string& message) : std::runtime_error(message), code_(code) {}
    Result code() const noexcept { return code_; }

private:
    Result code_;
};

enum class LogLevel : int32_t { Trace = 0, Debug = 1, Info = 2, Warn = 3, Error = 4, Off = 5 };

/// Called for every message at or above the log level. May be called from any thread.
/// The default sink writes to stderr. Pass an empty function to restore it.
using LogCallback = std::function<void(LogLevel level, const char* message)>;

RIS_API void set_log_callback(LogCallback callback);
RIS_API void set_log_level(LogLevel level) noexcept;

/// Monotonic seconds since the first call to any time function.
RIS_API double time_seconds() noexcept;
RIS_API void sleep_ms(uint32_t milliseconds) noexcept;

} // namespace ris
