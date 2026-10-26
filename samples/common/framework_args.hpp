#pragma once
// Command line options shared by the framework samples.
//
//   --frames N         render N frames and exit (default: until the window is closed)
//   --screenshot FILE  write the last frame to FILE as a binary PPM (needs --frames)
//   --size WxH         window size
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

inline std::string asset(const char* sample_name, const char* file) {
    return std::string(RIS_SAMPLES_DIR) + "/" + sample_name + "/assets/" + file;
}

inline void apply_args(RisFrameworkConfiguration& config, int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--size") && i + 1 < argc) {
            int w = 0, h = 0;
            if (std::sscanf(argv[++i], "%dx%d", &w, &h) == 2) {
                config.backBufferWidth = static_cast<uint32_t>(w);
                config.backBufferHeight = static_cast<uint32_t>(h);
            }
        }
    }
}

/// Call after the listeners of the sample are added and before `initialize()`: `--frames N` quits after N frames
/// (with a fixed 60 Hz time step, so runs are reproducible), `--screenshot FILE` writes the last of them.
inline void apply_run_args(ris::Framework& framework, int argc, char** argv) {
    int frames = -1;
    const char* screenshot = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--frames") && i + 1 < argc) {
            frames = std::atoi(argv[++i]);
        } else if (!std::strcmp(argv[i], "--screenshot") && i + 1 < argc) {
            screenshot = argv[++i];
        }
    }
    if (frames < 0) return;
    framework.timeManager().set_deterministic_step_ms(1000.0 / 60.0);
    framework.addOnRenderListener([frames, screenshot](ris::Framework& f) {
        if (f.timeManager().time().frame_number < static_cast<uint64_t>(frames)) return;
        if (screenshot) {
            const glm::vec2 size = f.renderer().back_buffer_size();
            const int w = static_cast<int>(size.x), h = static_cast<int>(size.y);
            const std::vector<uint8_t> px = ris::gfx::read_pixels(0, 0, w, h);
            if (std::FILE* file = std::fopen(screenshot, "wb")) {
                std::fprintf(file, "P6\n%d %d\n255\n", w, h);
                for (int y = h - 1; y >= 0; --y) {   // OpenGL's origin is the bottom left
                    for (int x = 0; x < w; ++x) std::fwrite(&px[(static_cast<size_t>(y) * w + x) * 4], 1, 3, file);
                }
                std::fclose(file);
            } else {
                std::fprintf(stderr, "could not write %s\n", screenshot);
            }
        }
        f.quit();
    });
}

} // namespace sample
