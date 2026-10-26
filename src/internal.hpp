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



/// Calls the listeners of a copy of the map, so a listener may add or remove listeners; a listener removed meanwhile is skipped.
template <class Map, class... Args>
void dispatch(const Map& listeners, Args&&... args) {
    const Map copy = listeners;
    for (const auto& [id, listener] : copy) {
        if (listeners.contains(id)) listener(args...);
    }
}

} // namespace ris::detail
