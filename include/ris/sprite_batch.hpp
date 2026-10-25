#pragma once

#include <cstdint>
#include <memory>

#include "graphics.hpp"

namespace ris {

/// Same layout as the C API's `ris_sprite`.
struct Sprite {
    const Texture* texture = nullptr;   ///< null = 1x1 white
    float x = 0, y = 0;                 ///< position of the origin point
    float width = 0, height = 0;
    float u0 = 0, v0 = 0, u1 = 1, v1 = 1;   ///< source rectangle in texture coordinates (0,0 = top left)
    float r = 1, g = 1, b = 1, a = 1;       ///< tint
    float rotation = 0;                 ///< radians, counter clockwise on screen
    float origin_x = 0, origin_y = 0;   ///< pivot in pixels relative to the sprite's top left corner
};

/// Batches textured quads into as few draw calls as possible.
class RIS_API SpriteBatch {
public:
    explicit SpriteBatch(uint32_t max_sprites_per_flush = 2048);
    ~SpriteBatch();
    SpriteBatch(SpriteBatch&&) noexcept;
    SpriteBatch& operator=(SpriteBatch&&) noexcept;
    SpriteBatch(const SpriteBatch&) = delete;
    SpriteBatch& operator=(const SpriteBatch&) = delete;

    /// `view_projection`: column-major mat4, or null for a pixel space projection of the current
    /// viewport with (0,0) at the top left and y pointing down.
    void begin(const float* view_projection = nullptr, BlendMode blend = BlendMode::Alpha);
    void draw(const Sprite& sprite);
    /// Textured, tinted, axis aligned quad covering the whole texture.
    void draw_quad(const Texture* texture, float x, float y, float width, float height, float r = 1, float g = 1,
                   float b = 1, float a = 1);
    void end();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace ris
