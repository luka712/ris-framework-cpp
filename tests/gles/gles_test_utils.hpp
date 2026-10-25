#pragma once

// Shared helpers of the tests of the gles layer. The GL tests need a display (or Xvfb) and an EGL
// implementation like test_window.cpp.

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

#include <ris/ris.hpp>

#include "../rendering/buffers/buffer_test_utils.hpp"

namespace ris_test {

using Pixel = std::array<int, 4>;

/// Reads one RGBA pixel of the bound framebuffer.
inline Pixel read_pixel(int x, int y) {
    const auto px = ris::gfx::read_pixels(x, y, 1, 1);
    return {px[0], px[1], px[2], px[3]};
}

/// Draws one triangle that covers the whole viewport, in a single colour. The triangle is counter clockwise.
class FullscreenTriangle {
public:
    FullscreenTriangle()
        : vertices_(std::vector<float>{-1, -1, 3, -1, -1, 3}, 8),
          vao_(ris::gles::make_vertex_array_desc(vertices_, layout())),
          shader_(R"(#version 300 es
                layout(location = 0) in vec2 a_position;
                void main() { gl_Position = vec4(a_position, 0.0, 1.0); }
            )",
                  R"(#version 300 es
                precision mediump float;
                uniform vec4 u_color;
                out vec4 frag;
                void main() { frag = u_color; }
            )"),
          color_location_(shader_.uniform_location("u_color")) {}

    void draw(float r, float g, float b, float a) {
        shader_.set_vec4(color_location_, r, g, b, a);
        vao_.draw(ris::Topology::Triangles, 0, 3);
    }

private:
    static ris::VertexBufferLayout layout() {
        ris::VertexBufferLayout l;
        l.array_stride = 8;
        l.attributes = {{ris::VertexFormat::Float32x2, 0, 0}};
        return l;
    }

    ris::gles::GlesVertexBuffer vertices_;
    ris::VertexArray vao_;
    ris::Shader shader_;
    int32_t color_location_;
};

} // namespace ris_test
