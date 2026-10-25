// The same image (a 248x248 UASTC .ktx2) as three KTX2 files side by side, loaded with ris::Texture::from_ktx2_file:
//
//   left    ASTC 4x4   (Basis transcode target when GL_KHR_texture_compression_astc_ldr is available)
//   middle  ETC2 RGBA  (Basis transcode target otherwise; core in OpenGL ES 3)
//   right   RGBA8      (uncompressed reference, the UASTC data decoded without any GPU compression)
//
// BC7 is not used. The framework has no text rendering, so the panels are labelled on the console and in the
// window title. A variant the GL context cannot use is skipped with a message and drawn as a red placeholder.
//
//   --scale N   display each 248x248 texture N times larger (default 2)
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>

#include "../common/sample_app.hpp"

namespace {
constexpr int kTexSize = 248;
constexpr int kGap = 24;
constexpr int kMargin = 24;

struct Panel {
    const char* label;
    const char* file;
    int bytes_per_pixel;        // GPU memory: ASTC 4x4 and ETC2 RGBA use 16 bytes per 4x4 block
    std::unique_ptr<ris::Texture> texture;
    std::string status;
};

int parse_scale(int argc, char** argv) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (!std::strcmp(argv[i], "--scale")) return std::max(1, std::atoi(argv[i + 1]));
    }
    return 2;
}
} // namespace

int main(int argc, char** argv) {
    try {
        const int scale = parse_scale(argc, argv);
        const int tile = kTexSize * scale;
        const int window_w = 3 * tile + 2 * kGap + 2 * kMargin;
        const int window_h = tile + 2 * kMargin;
        sample::App app(argc, argv, "ris sample: ktx2_compare", window_w, window_h);

        const bool astc = app.window().has_gl_extension("GL_KHR_texture_compression_astc_ldr");
        std::cout << "GL_KHR_texture_compression_astc_ldr: " << (astc ? "yes" : "no")
                  << "  (ETC2 is core in OpenGL ES 3, BC7 is not used)\n";

        Panel panels[3] = {
            {"ASTC 4x4 RGBA", "test_astc4x4.ktx2", 1, nullptr, {}},
            {"ETC2 RGBA", "test_etc2.ktx2", 1, nullptr, {}},
            {"RGBA8 reference", "test_rgba8.ktx2", 4, nullptr, {}},
        };
        std::string title = "ris sample: ktx2_compare | left to right:";
        for (Panel& p : panels) {
            try {
                p.texture = std::make_unique<ris::Texture>(ris::Texture::from_ktx2_file(sample::asset("ktx2_compare", p.file)));
                p.texture->set_sampler({.min_filter = ris::Filter::Linear, .mag_filter = ris::Filter::Linear});
                const long kib = static_cast<long>(p.texture->width()) * p.texture->height() * p.bytes_per_pixel / 1024;
                p.status = "ok, " + std::to_string(p.texture->width()) + "x" + std::to_string(p.texture->height()) + ", ~" +
                           std::to_string(kib) + " KiB of GPU memory";
            } catch (const std::exception& e) {
                p.status = std::string("SKIPPED: ") + e.what();
            }
            std::cout << "  " << p.label << " (" << p.file << "): " << p.status << "\n";
            title += std::string(" ") + p.label + (p.texture ? "," : " (skipped),");
        }
        title.pop_back();
        app.window().set_title(title);

        ris::SpriteBatch sprite_batch;

        while (app.begin_frame()) {
            ris::gfx::clear(0.10f, 0.12f, 0.16f, 1.0f);

            sprite_batch.begin();
            for (int i = 0; i < 3; ++i) {
                const float x = static_cast<float>(kMargin + i * (tile + kGap));
                const float y = static_cast<float>(kMargin);
                // Opaque backing, so that the alpha channel of every variant is compared against the same colour.
                sprite_batch.draw_quad(nullptr, x, y, static_cast<float>(tile), static_cast<float>(tile), 0.30f, 0.30f, 0.34f, 1.0f);
                if (panels[i].texture) {
                    sprite_batch.draw_quad(panels[i].texture.get(), x, y, static_cast<float>(tile), static_cast<float>(tile));
                } else {
                    sprite_batch.draw_quad(nullptr, x, y, static_cast<float>(tile), static_cast<float>(tile), 0.6f, 0.1f, 0.1f, 1.0f);
                }
            }
            sprite_batch.end();

            app.end_frame();
        }
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
