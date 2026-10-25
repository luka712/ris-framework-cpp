// Loads a Basis Universal (UASTC) .ktx2 file with ris::Texture::from_ktx2_file and shows it as a textured quad.
//
// The file is transcoded to the best format the GPU supports (ASTC, then ETC2, then RGBA8), see README.md.
//
//   --scale N   display the 248x248 texture N times larger (default 2)
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "../common/sample_app.hpp"

namespace {
constexpr int kMargin = 32;     // pixels around the quad
constexpr int kCheckerSize = 16;

int parse_scale(int argc, char** argv) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (!std::strcmp(argv[i], "--scale")) return std::max(1, std::atoi(argv[i + 1]));
    }
    return 2;
}
} // namespace

int main(int argc, char** argv) {
    try {
        // The texture is 248x248, so the window size can be derived from the scale before it is loaded.
        const int scale = parse_scale(argc, argv);
        const int window_size = 248 * scale + 2 * kMargin;
        sample::App app(argc, argv, "ris sample: ktx2_quad", window_size, window_size);

        ris::Texture texture = ris::Texture::from_ktx2_file(sample::asset("ktx2_quad", "test_basis_uastc.ktx2"));
        texture.set_sampler({.min_filter = ris::Filter::Linear, .mag_filter = ris::Filter::Linear});
        std::cout << "Loaded test_basis_uastc.ktx2 (" << texture.width() << "x" << texture.height() << ", "
                  << texture.mip_levels() << " level(s)); ASTC "
                  << (app.window().has_gl_extension("GL_KHR_texture_compression_astc_ldr") ? "supported, transcoded to ASTC 4x4"
                                                                                           : "not supported, transcoded to ETC2")
                  << "\n";

        ris::SpriteBatch sprite_batch;

        while (app.begin_frame()) {
            ris::gfx::clear(0.10f, 0.12f, 0.16f, 1.0f);

            sprite_batch.begin();
            // The texture has an alpha channel, so a checkerboard behind the quad makes the blending visible.
            const int w = app.width(), h = app.height();
            for (int y = 0; y * kCheckerSize < h; ++y) {
                for (int x = 0; x * kCheckerSize < w; ++x) {
                    if (((x + y) & 1) == 0) continue;
                    sprite_batch.draw_quad(nullptr, static_cast<float>(x * kCheckerSize), static_cast<float>(y * kCheckerSize),
                                           kCheckerSize, kCheckerSize, 0.16f, 0.19f, 0.24f, 1.0f);
                }
            }
            sprite_batch.draw_quad(&texture, kMargin, kMargin, static_cast<float>(texture.width() * scale),
                                   static_cast<float>(texture.height() * scale));
            sprite_batch.end();

            app.end_frame();
        }
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
