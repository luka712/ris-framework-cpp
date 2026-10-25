// Port of the `sprite_batch` example of ris-framework-ts: load a texture and draw it with the sprite batch.
#include <iostream>

#include "../common/framework_args.hpp"

int main(int argc, char** argv) {
    try {
        ris::FrameworkConfiguration config;
        config.title = "ris sample: sprite_batch";
        config.backBufferSize = {560, 380};
        config.textureFiltering = ris::TextureFilteringPreset::Bilinear;
        sample::apply_args(config, argc, argv);

        ris::Framework framework(config);

        ris::Texture* parrots = nullptr;   // owned by framework.content()
        ris::Rect draw_rect(16, 16, 512, 341);
        const ris::Color color = ris::Color::white();

        framework.addOnLoadContentListener([&](ris::Framework&) {
            // Kodak test image kodim23 (parrots), 512x341. See README.md for source and license.
            parrots = framework.content().loadTexture2D(sample::asset("sprite_batch", "kodim23.png"));
            draw_rect.width = static_cast<float>(parrots->width());
            draw_rect.height = static_cast<float>(parrots->height());
            std::cout << "Loaded kodim23.png (" << parrots->width() << "x" << parrots->height() << ").\n";
        });

        framework.addOnRenderListener([&](ris::Framework&) {
            if (!parrots) return;
            ris::SpriteBatch& sprite_batch = framework.spriteBatch();
            sprite_batch.begin();
            sprite_batch.draw(*parrots, draw_rect, color);
            sprite_batch.end();
        });

        framework.renderer().setClearColor(ris::Color::light_pink());
        framework.initialize();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
