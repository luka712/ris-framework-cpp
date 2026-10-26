#include "../include/ris/sprites/sprite_batch.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

#include "internal.hpp"
#include "ris/math.hpp"

using namespace ris::detail;

namespace ris {

namespace {

struct Vertex {
    float x, y;
    float u, v;
    uint8_t r, g, b, a;
};
static_assert(sizeof(Vertex) == 20, "sprite vertex must be tightly packed");

constexpr uint32_t kMaxSpritesLimit = 16384;   // 4 vertices each must fit uint16 indices

const char* kVertexSource = R"(#version 300 es
uniform mat4 u_view_projection;
layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec4 a_color;
out vec2 v_uv;
out vec4 v_color;
void main() {
    v_uv = a_uv;
    v_color = a_color;
    gl_Position = u_view_projection * vec4(a_position, 0.0, 1.0);
}
)";

const char* kFragmentSource = R"(#version 300 es
precision mediump float;
uniform sampler2D u_texture;
in vec2 v_uv;
in vec4 v_color;
out vec4 o_color;
void main() {
    o_color = texture(u_texture, v_uv) * v_color;
}
)";

uint8_t to_byte(float v) {
    return static_cast<uint8_t>(std::lround(std::clamp(v, 0.0f, 1.0f) * 255.0f));
}

} // namespace

struct ASpriteBatch::Impl {
    uint32_t max_sprites;
    Shader shader;
    Buffer vertex_buffer;
    Buffer index_buffer;
    VertexArray mesh;
    gles::GlesTexture2D white;
    int32_t u_view_projection;
    int32_t u_texture;

    std::vector<Vertex> vertices;
    const gles::GlesTexture2D* current = nullptr;
    bool active = false;

    static VertexArrayDesc make_mesh_desc(const Buffer& vb, const Buffer& ib) {
        VertexArrayDesc desc;
        desc.attribs = {
            {0, 2, AttribType::Float, false, offsetof(Vertex, x), 0, 0, false},
            {1, 2, AttribType::Float, false, offsetof(Vertex, u), 0, 0, false},
            {2, 4, AttribType::UByte, true, offsetof(Vertex, r), 0, 0, false},
        };
        desc.vertex_buffers[0] = {&vb, sizeof(Vertex)};
        desc.index_buffer = &ib;
        desc.index_type = IndexType::UInt16;
        return desc;
    }

    static std::vector<uint16_t> make_indices(uint32_t sprites) {
        std::vector<uint16_t> idx(static_cast<size_t>(sprites) * 6);
        for (uint32_t i = 0; i < sprites; ++i) {
            const auto base = static_cast<uint16_t>(i * 4);
            uint16_t* p = &idx[static_cast<size_t>(i) * 6];
            p[0] = base;
            p[1] = static_cast<uint16_t>(base + 1);
            p[2] = static_cast<uint16_t>(base + 2);
            p[3] = static_cast<uint16_t>(base + 2);
            p[4] = static_cast<uint16_t>(base + 1);
            p[5] = static_cast<uint16_t>(base + 3);
        }
        return idx;
    }

    static const uint8_t* white_pixel() {
        static const uint8_t px[4] = {255, 255, 255, 255};
        return px;
    }

    explicit Impl(uint32_t max)
        : max_sprites(std::clamp<uint32_t>(max, 1, kMaxSpritesLimit)),
          shader(kVertexSource, kFragmentSource),
          vertex_buffer(BufferType::Vertex, BufferUsage::Stream, nullptr, sizeof(Vertex) * 4 * max_sprites),
          index_buffer(BufferType::Index, BufferUsage::Static, make_indices(max_sprites).data(),
                       sizeof(uint16_t) * 6 * max_sprites),
          mesh(make_mesh_desc(vertex_buffer, index_buffer)),
          white(gles::GlesTexture2D::from_rgba8(1, 1, white_pixel(), false, false)),
          u_view_projection(shader.uniform_location("u_view_projection")),
          u_texture(shader.uniform_location("u_texture")) {
        vertices.reserve(static_cast<size_t>(max_sprites) * 4);
    }

    void flush() {
        if (vertices.empty()) return;
        const size_t sprite_count = vertices.size() / 4;
        vertex_buffer.update(0, vertices.data(), vertices.size() * sizeof(Vertex));
        (current ? *current : white).bind(0);
        mesh.draw(Topology::Triangles, 0, static_cast<uint32_t>(sprite_count * 6), 1);
        vertices.clear();
    }

    void push(const Sprite& s) {
        const gles::GlesTexture2D* tex = s.texture;
        if (tex != current && !vertices.empty()) flush();
        current = tex;
        if (vertices.size() / 4 >= max_sprites) flush();

        const float cos_r = std::cos(s.rotation);
        const float sin_r = std::sin(s.rotation);
        const float xs[4] = {0.0f, s.width, 0.0f, s.width};
        const float ys[4] = {0.0f, 0.0f, s.height, s.height};
        const float us[4] = {s.u0, s.u1, s.u0, s.u1};
        const float vs[4] = {s.v0, s.v0, s.v1, s.v1};
        const uint8_t r = to_byte(s.r), g = to_byte(s.g), b = to_byte(s.b), a = to_byte(s.a);
        for (int i = 0; i < 4; ++i) {
            const float lx = xs[i] - s.origin_x;
            const float ly = ys[i] - s.origin_y;
            // y points down on screen, so a counter clockwise rotation is the transposed matrix.
            vertices.push_back({s.x + lx * cos_r + ly * sin_r, s.y - lx * sin_r + ly * cos_r, us[i], vs[i], r, g, b, a});
        }
    }
};

ASpriteBatch::ASpriteBatch(uint32_t max_sprites_per_flush) {

    impl_ = std::make_unique<Impl>(max_sprites_per_flush);
}

ASpriteBatch::~ASpriteBatch() = default;
ASpriteBatch::ASpriteBatch(ASpriteBatch&&) noexcept = default;
ASpriteBatch& ASpriteBatch::operator=(ASpriteBatch&&) noexcept = default;

void ASpriteBatch::begin(const float* view_projection, BlendMode blend) {

    if (impl_->active) {
        throw std::invalid_argument("SpriteBatch::begin called twice without end()");
    }
    impl_->active = true;
    impl_->current = nullptr;
    impl_->vertices.clear();

    glm::mat4 pixel_projection;
    if (!view_projection) {
        GLint vp[4] = {0, 0, 1, 1};
        glGetIntegerv(GL_VIEWPORT, vp);
        pixel_projection = glm::ortho(0.0f, static_cast<float>(vp[2]), static_cast<float>(vp[3]), 0.0f, -1.0f, 1.0f);
        view_projection = glm::value_ptr(pixel_projection);
    }

    gfx::set_blend_mode(blend);
    gfx::set_depth_state(false, false);
    gfx::set_cull_mode(CullMode::None);
    impl_->shader.use();
    impl_->shader.set_mat4(impl_->u_view_projection, view_projection);
    impl_->shader.set_int(impl_->u_texture, 0);
}

void ASpriteBatch::draw(const Sprite& sprite) {
    if (!impl_->active) {
        throw std::invalid_argument("SpriteBatch::draw called outside begin()/end()");
    }
    impl_->push(sprite);
}

void ASpriteBatch::draw_quad(const gles::GlesTexture2D* texture, float x, float y, float width, float height, float r, float g,
                            float b, float a) {
    Sprite s;
    s.texture = texture;
    s.x = x;
    s.y = y;
    s.width = width;
    s.height = height;
    s.r = r;
    s.g = g;
    s.b = b;
    s.a = a;
    draw(s);
}

void ASpriteBatch::end() {
    if (!impl_->active) {
        throw std::invalid_argument("SpriteBatch::end called without begin()");
    }
    impl_->flush();
    impl_->active = false;
}

} // namespace ris

namespace ris {

void ASpriteBatch::draw(const gles::GlesTexture2D& texture, const Rect& dest, const Color& color, std::optional<Rect> source,
                       float rotation, std::optional<glm::vec2> origin, float layer_depth) {
    (void)layer_depth;
    Sprite s;
    s.texture = &texture;
    const glm::vec2 o = origin.value_or(glm::vec2(0, 0));
    s.x = dest.x + o.x;
    s.y = dest.y + o.y;
    s.width = dest.width;
    s.height = dest.height;
    if (source) {
        const float tw = static_cast<float>(std::max(1, texture.getWidth()));
        const float th = static_cast<float>(std::max(1, texture.getHeight()));
        s.u0 = source->x / tw;
        s.v0 = source->y / th;
        s.u1 = (source->x + source->width) / tw;
        s.v1 = (source->y + source->height) / th;
    }
    s.r = color.r;
    s.g = color.g;
    s.b = color.b;
    s.a = color.a;
    s.rotation = rotation;
    s.origin_x = o.x;
    s.origin_y = o.y;
    draw(s);
}

void ASpriteBatch::draw_rect(const Rect& dest, const Color& color, float rotation, std::optional<glm::vec2> origin,
                            float layer_depth) {
    (void)layer_depth;
    Sprite s;   // null texture = 1x1 white
    const glm::vec2 o = origin.value_or(glm::vec2(0, 0));
    s.x = dest.x + o.x;
    s.y = dest.y + o.y;
    s.width = dest.width;
    s.height = dest.height;
    s.r = color.r;
    s.g = color.g;
    s.b = color.b;
    s.a = color.a;
    s.rotation = rotation;
    s.origin_x = o.x;
    s.origin_y = o.y;
    draw(s);
}

} // namespace ris
