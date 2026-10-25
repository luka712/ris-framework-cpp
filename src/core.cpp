#include "internal.hpp"
#include "ris/gles/utilities/error_utilities.hpp"

#include <SDL3/SDL.h>

#include <atomic>
#include <cstdio>
#include <mutex>

namespace {

struct LogState {
    std::mutex mutex;
    ris::LogCallback callback;
};

LogState& log_state() {
    static LogState state;
    return state;
}

std::atomic<int32_t> g_log_level{static_cast<int32_t>(ris::LogLevel::Info)};
std::atomic<bool> g_context_alive{false};

const char* level_name(ris::LogLevel level) {
    switch (level) {
        case ris::LogLevel::Trace: return "trace";
        case ris::LogLevel::Debug: return "debug";
        case ris::LogLevel::Info: return "info";
        case ris::LogLevel::Warn: return "warn";
        case ris::LogLevel::Error: return "error";
        default: return "?";
    }
}

std::string vformat(const char* fmt, va_list args) {
    va_list copy;
    va_copy(copy, args);
    const int len = std::vsnprintf(nullptr, 0, fmt, copy);
    va_end(copy);
    if (len <= 0) {
        return {};
    }
    std::string out(static_cast<size_t>(len), '\0');
    std::vsnprintf(out.data(), out.size() + 1, fmt, args);
    return out;
}

uint64_t time_origin_ns() {
    static const uint64_t origin = SDL_GetTicksNS();
    return origin;
}

} // namespace

namespace ris::detail {

void log(LogLevel level, const char* fmt, ...) {
    if (static_cast<int32_t>(level) < g_log_level.load(std::memory_order_relaxed)) {
        return;
    }
    va_list args;
    va_start(args, fmt);
    const std::string message = vformat(fmt, args);
    va_end(args);

    LogState& state = log_state();
    std::lock_guard<std::mutex> lock(state.mutex);
    if (state.callback) {
        state.callback(level, message.c_str());
    } else {
        std::fprintf(stderr, "[ris] [%s] %s\n", level_name(level), message.c_str());
    }
}

std::string format(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    std::string out = vformat(fmt, args);
    va_end(args);
    return out;
}

void throw_error(Result code, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    std::string message = vformat(fmt, args);
    va_end(args);
    throw Error(code, message);
}

void require_context() {
    if (!g_context_alive.load(std::memory_order_acquire)) {
        throw_error(Result::NotInitialized, "no ris::Window exists: create a Window before using graphics objects");
    }
}

bool has_context() noexcept { return g_context_alive.load(std::memory_order_acquire); }
void set_context_alive(bool alive) noexcept { g_context_alive.store(alive, std::memory_order_release); }

void check_gl(const char* what) { gles::error_utilities::check_error(what); }

} // namespace ris::detail

namespace ris {

const char* version_string() noexcept { return "0.1.0"; }

void set_log_callback(LogCallback callback) {
    LogState& state = log_state();
    std::lock_guard<std::mutex> lock(state.mutex);
    state.callback = std::move(callback);
}

void set_log_level(LogLevel level) noexcept {
    g_log_level.store(static_cast<int32_t>(level), std::memory_order_relaxed);
}

double time_seconds() noexcept {
    const uint64_t origin = time_origin_ns();
    return static_cast<double>(SDL_GetTicksNS() - origin) * 1e-9;
}

void sleep_ms(uint32_t milliseconds) noexcept { SDL_Delay(milliseconds); }

} // namespace ris
