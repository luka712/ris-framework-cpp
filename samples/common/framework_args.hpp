#pragma once
// Command line options shared by the framework samples.
//
//   --frames N         render N frames and exit (default: until the window is closed)
//   --hidden           hidden window, for automated runs
//   --screenshot FILE  write the last frame to FILE as a binary PPM (needs --frames)
//   --size WxH         window size
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include <ris/ris.hpp>

#ifndef RIS_SAMPLES_DIR
#define RIS_SAMPLES_DIR "."
#endif

namespace sample {

inline std::string asset(const char* sample_name, const char* file) {
    return std::string(RIS_SAMPLES_DIR) + "/" + sample_name + "/assets/" + file;
}

inline void apply_args(ris::FrameworkConfiguration& config, int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--frames") && i + 1 < argc) {
            config.max_frames = std::atoi(argv[++i]);
        } else if (!std::strcmp(argv[i], "--hidden")) {
            config.hidden = true;
        } else if (!std::strcmp(argv[i], "--screenshot") && i + 1 < argc) {
            config.screenshot_path = argv[++i];
        } else if (!std::strcmp(argv[i], "--size") && i + 1 < argc) {
            int w = 0, h = 0;
            if (std::sscanf(argv[++i], "%dx%d", &w, &h) == 2) config.backBufferSize = {static_cast<float>(w), static_cast<float>(h)};
        }
    }
}

} // namespace sample
