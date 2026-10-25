// Port of the `sprite_batch` example of ris-framework-ts: load a texture and draw it with the sprite batch.
#include <iostream>

#include "../common/framework_args.hpp"

int main(int argc, char** argv) {
    try {
        ris::FrameworkConfiguration config;
        config.title = "ris sample: sprite_batch";
        config.back_buffer_size = {560, 380};
        config.texture_filtering = ris::TextureFilteringPreset::Bilinear;
        sample::apply_args(config, argc, argv);

        ris::Framework framework(config);

        std::shared_ptr<ris::Texture> parrots;
        ris::Rect draw_rect(16, 16, 512, 341);
        const ris::Color color = ris::Color::white();

        framework.add_on_load_content_listener([&] {
            // Kodak test image kodim23 (parrots), 512x341. See README.md for source and license.
            parrots = framework.content().load_texture_2d(sample::asset("sprite_batch", "kodim23.png"));
            draw_rect.width = static_cast<float>(parrots->width());
            draw_rect.height = static_cast<float>(parrots->height());
            std::cout << "Loaded kodim23.png (" << parrots->width() << "x" << parrots->height() << ").\n";
        });

        framework.add_on_render_listener([&] {
            if (!parrots) return;
            ris::SpriteBatch& sprite_batch = framework.sprite_batch();
            sprite_batch.begin();
            sprite_batch.draw(*parrots, draw_rect, color);
            sprite_batch.end();
        });

        framework.renderer().clear_color = ris::Color::light_pink();
        framework.initialize();
        parrots.reset();   // GPU objects must go before the framework
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
