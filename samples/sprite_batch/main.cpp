// Port of the `sprite_batch` example of ris-framework-ts: load a texture and draw it with the sprite batch.
#include <iostream>

#include "../common/framework_args.hpp"

int main(int argc, char** argv) {
    try {
        RisFrameworkConfiguration config {};
        config.title = "ris sample: sprite_batch";
        config.backBufferWidth = 1280;
        config.backBufferHeight = 720;
        config.textureFiltering = RIS_TEXTURE_FILTERING_BILINEAR;
        sample::apply_args(config, argc, argv);

        ris::Framework framework(config);

        ris::gles::GlesTexture2D* parrots = nullptr;   // owned by framework.content()
        ris::Rect draw_rect(16, 16, 512, 341);
        const ris::Color color = ris::Color::white();

        framework.addOnLoadContentListener([&](ris::Framework&) {
            // Kodak test image kodim23 (parrots), 512x341. See README.md for source and license.
            parrots = framework.content().loadTexture2D(sample::asset("sprite_batch", "kodim23.png"));
            draw_rect.width = static_cast<float>(parrots->getWidth());
            draw_rect.height = static_cast<float>(parrots->getHeight());
            std::cout << "Loaded kodim23.png (" << parrots->getWidth() << "x" << parrots->getHeight() << ").\n";
        });

        framework.addOnRenderListener([&](ris::Framework&) {
            if (!parrots) return;
            ris::ASpriteBatch& sprite_batch = framework.spriteBatch();
            sprite_batch.begin();
            sprite_batch.draw(*parrots, draw_rect, color);
            sprite_batch.end();
        });

        framework.renderer().setClearColor(ris::Color::lightPink());
        sample::apply_run_args(framework, argc, argv);
        framework.initialize();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
