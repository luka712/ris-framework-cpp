// Port of the `sprite_batch` example of ris-framework-ts / RisFramework.Demo:
// texture loading and drawing a single sprite with the sprite batch.
#include <iostream>

#include "../common/sample_app.hpp"

int main(int argc, char** argv) {
    try {
        sample::App app(argc, argv, "ris sample: sprite_batch", 560, 380);

        // Kodak test image kodim23 (parrots), 512x341. See README.md for source and license.
        // Not sRGB: the sprite shader writes the sampled texel to a non sRGB framebuffer unchanged.
        ris::Texture parrots = ris::Texture::from_image_file(sample::asset("sprite_batch", "kodim23.png"),
                                                             /*generate_mipmaps=*/true, /*srgb=*/false);
        parrots.set_sampler({.min_filter = ris::Filter::LinearMipmapLinear, .mag_filter = ris::Filter::Linear});
        std::cout << "Loaded kodim23.png (" << parrots.width() << "x" << parrots.height() << ")\n";

        ris::SpriteBatch sprite_batch;

        while (app.begin_frame()) {
            ris::gfx::clear(1.0f, 0.714f, 0.757f, 1.0f);   // light pink

            sprite_batch.begin();
            sprite_batch.draw_quad(&parrots, 16, 16, static_cast<float>(parrots.width()), static_cast<float>(parrots.height()));
            sprite_batch.end();

            app.end_frame();
        }
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
