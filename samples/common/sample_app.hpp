#pragma once
// Small helper shared by the samples: command line options, the frame loop and screenshots.

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include <ris/ris.hpp>

#ifndef RIS_SAMPLES_DIR
#define RIS_SAMPLES_DIR "."
#endif

namespace sample {

/// Absolute path of a file below samples/<sample>/assets.
inline std::string asset(const char* sample_name, const char* file) {
    return std::string(RIS_SAMPLES_DIR) + "/" + sample_name + "/assets/" + file;
}

/// Window + frame loop.
///
///   --frames N         render N frames and exit (default: until the window is closed)
///   --hidden           hidden window, for automated runs
///   --screenshot FILE  write the last frame to FILE as a binary PPM
///   --size WxH         window size
class App {
public:
    App(int argc, char** argv, const char* title, int width = 960, int height = 540) {
        ris::WindowDesc desc;
        desc.title = title;
        desc.width = width;
        desc.height = height;
        desc.flags = ris::WindowFlags::Resizable | ris::WindowFlags::HighDpi;
        for (int i = 1; i < argc; ++i) {
            if (!std::strcmp(argv[i], "--frames") && i + 1 < argc) {
                max_frames_ = std::atoi(argv[++i]);
            } else if (!std::strcmp(argv[i], "--hidden")) {
                desc.flags = desc.flags | ris::WindowFlags::Hidden;
            } else if (!std::strcmp(argv[i], "--screenshot") && i + 1 < argc) {
                screenshot_ = argv[++i];
            } else if (!std::strcmp(argv[i], "--size") && i + 1 < argc) {
                std::sscanf(argv[++i], "%dx%d", &desc.width, &desc.height);
            }
        }
        window_ = std::make_unique<ris::Window>(desc);
        std::printf("%s | %s | %s\n", title, window_->gl_info(ris::GlInfo::Renderer), window_->gl_info(ris::GlInfo::Version));
        last_time_ = ris::time_seconds();
    }

    ris::Window& window() { return *window_; }
    int width() const { int32_t w, h; window_->drawable_size(w, h); return w; }
    int height() const { int32_t w, h; window_->drawable_size(w, h); return h; }
    float aspect() const { return static_cast<float>(width()) / static_cast<float>(std::max(1, height())); }

    float delta_time() const { return delta_; }
    float elapsed_time() const { return static_cast<float>(elapsed_); }

    /// Pumps events and advances the clock. Returns false once the sample should stop. Also sets the
    /// viewport to the whole window.
    bool begin_frame() {
        ris::Event ev;
        while (window_->poll_event(ev)) {
            if (ev.type == ris::EventType::Quit) quit_ = true;
            if (ev.type == ris::EventType::KeyDown && ev.scancode == 41 /* Escape */) quit_ = true;
        }
        if (quit_ || (max_frames_ >= 0 && frame_ >= max_frames_)) return false;
        const double now = ris::time_seconds();
        delta_ = static_cast<float>(now - last_time_);
        last_time_ = now;
        elapsed_ += delta_;
        // Fixed time step for automated runs, so screenshots are reproducible.
        if (max_frames_ >= 0) {
            delta_ = 1.0f / 60.0f;
            elapsed_ = static_cast<double>(frame_) / 60.0;
        }
        ris::gfx::viewport(0, 0, width(), height());
        return true;
    }

    void end_frame() {
        ++frame_;
        const bool last = max_frames_ >= 0 && frame_ >= max_frames_;
        if (last && !screenshot_.empty()) save_screenshot(screenshot_);
        window_->swap_buffers();
    }

private:
    void save_screenshot(const std::string& path) {
        const int w = width(), h = height();
        const std::vector<uint8_t> px = ris::gfx::read_pixels(0, 0, w, h);
        std::FILE* f = std::fopen(path.c_str(), "wb");
        if (!f) {
            std::fprintf(stderr, "could not write %s\n", path.c_str());
            return;
        }
        std::fprintf(f, "P6\n%d %d\n255\n", w, h);
        for (int y = h - 1; y >= 0; --y) {            // GL origin is bottom left
            for (int x = 0; x < w; ++x) std::fwrite(&px[(static_cast<size_t>(y) * w + x) * 4], 1, 3, f);
        }
        std::fclose(f);
        std::printf("wrote %s (%dx%d)\n", path.c_str(), w, h);
    }

    std::unique_ptr<ris::Window> window_;
    int max_frames_ = -1;
    int frame_ = 0;
    bool quit_ = false;
    std::string screenshot_;
    double last_time_ = 0, elapsed_ = 0;
    float delta_ = 0;
};

} // namespace sample
