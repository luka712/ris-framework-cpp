// The C API: a thin extern "C" layer over the C++ API. Every function catches exceptions and
// translates them into a return value plus ris_get_last_error().

#include <cstddef>
#include <cstring>
#include <exception>
#include <new>
#include <string>

#include "internal.hpp"
#include "ris/ris.h"
#include "ris/ris.hpp"

// ---- opaque handle <-> C++ object -------------------------------------------------------------
// Handles are the C++ objects themselves, reinterpreted.

struct ris_window : ris::Window {
    using ris::Window::Window;
};
struct ris_buffer : ris::Buffer {
    using ris::Buffer::Buffer;
};
struct ris_shader : ris::Shader {
    using ris::Shader::Shader;
};
struct ris_mesh : ris::VertexArray {
    using ris::VertexArray::VertexArray;
};
struct ris_render_target : ris::RenderTarget {
    using ris::RenderTarget::RenderTarget;
};
struct ris_sprite_batch : ris::SpriteBatch {
    using ris::SpriteBatch::SpriteBatch;
};
// ris::Texture has a private constructor (factories only), so the handle is the object itself.
struct ris_texture {
    ris::Texture value;
};

// ---- layout checks: C structs that are reinterpreted ------------------------------------------

static_assert(sizeof(ris::Event) == sizeof(ris_event), "ris::Event and ris_event must match");
static_assert(offsetof(ris::Event, type) == offsetof(ris_event, type), "");
static_assert(offsetof(ris::Event, timestamp_ns) == offsetof(ris_event, timestamp_ns), "");
static_assert(offsetof(ris::Event, scancode) == offsetof(ris_event, scancode), "");
static_assert(offsetof(ris::Event, mods) == offsetof(ris_event, mods), "");
static_assert(offsetof(ris::Event, x) == offsetof(ris_event, x), "");
static_assert(offsetof(ris::Event, width) == offsetof(ris_event, width), "");
static_assert(offsetof(ris::Event, text) == offsetof(ris_event, text), "");
static_assert(sizeof(ris_event) == 96, "ris_event is 96 bytes: the C# bindings depend on it");
static_assert(sizeof(ris_sprite) == 72, "ris_sprite is 72 bytes with explicit tail padding");
static_assert(sizeof(ris_mesh_desc) == 88, "ris_mesh_desc is 88 bytes with explicit padding");
static_assert(sizeof(ris_sampler_desc) == 20, "ris_sampler_desc is 20 bytes");

static_assert(sizeof(ris::Sprite) == sizeof(ris_sprite), "ris::Sprite and ris_sprite must match");
static_assert(offsetof(ris::Sprite, x) == offsetof(ris_sprite, x), "");
static_assert(offsetof(ris::Sprite, u0) == offsetof(ris_sprite, u0), "");
static_assert(offsetof(ris::Sprite, r) == offsetof(ris_sprite, r), "");
static_assert(offsetof(ris::Sprite, rotation) == offsetof(ris_sprite, rotation), "");
static_assert(offsetof(ris::Sprite, origin_y) == offsetof(ris_sprite, origin_y), "");

static_assert(static_cast<int>(ris::Result::Ktx) == RIS_ERROR_KTX, "");
static_assert(static_cast<int>(ris::Result::Compile) == RIS_ERROR_COMPILE, "");
static_assert(static_cast<int>(ris::BlendMode::Multiply) == RIS_BLEND_MULTIPLY, "");
static_assert(static_cast<int>(ris::Topology::Points) == RIS_TOPOLOGY_POINTS, "");
static_assert(static_cast<int>(ris::AttribType::Int) == RIS_ATTRIB_INT, "");
static_assert(static_cast<int>(ris::EventType::MouseWheel) == RIS_EVENT_MOUSE_WHEEL, "");
static_assert(static_cast<int>(ris::Filter::LinearMipmapLinear) == RIS_FILTER_LINEAR_MIPMAP_LINEAR, "");
static_assert(static_cast<int>(ris::AngleBackend::Null) == RIS_ANGLE_BACKEND_NULL, "");
static_assert(ris::max_vertex_buffers == RIS_MAX_VERTEX_BUFFERS, "");

namespace {

thread_local std::string t_last_error;

ris_result fail(ris_result code, const char* message) {
    t_last_error = message;
    ris::detail::log(ris::LogLevel::Error, "%s", message);
    return code;
}

/// Runs `fn`, returning RIS_OK or the translated error.
template <typename Fn>
ris_result guard(Fn&& fn) noexcept {
    try {
        fn();
        return RIS_OK;
    } catch (const ris::Error& e) {
        return fail(static_cast<ris_result>(e.code()), e.what());
    } catch (const std::bad_alloc&) {
        return fail(RIS_ERROR_OUT_OF_MEMORY, "out of memory");
    } catch (const std::exception& e) {
        return fail(RIS_ERROR_PLATFORM, e.what());
    } catch (...) {
        return fail(RIS_ERROR_PLATFORM, "unknown error");
    }
}

/// Runs `fn` (returning T*), nullptr on failure.
template <typename T, typename Fn>
T* guard_ptr(Fn&& fn) noexcept {
    T* result = nullptr;
    const ris_result rc = guard([&] { result = fn(); });
    return rc == RIS_OK ? result : nullptr;
}

ris_result invalid(const char* what) { return fail(RIS_ERROR_INVALID_ARGUMENT, what); }

} // namespace

extern "C" {

// ---- core -----------------------------------------------------------------------------------

uint32_t RIS_CALL ris_get_version_major(void) { return RIS_VERSION_MAJOR; }
uint32_t RIS_CALL ris_get_version_minor(void) { return RIS_VERSION_MINOR; }
uint32_t RIS_CALL ris_get_version_patch(void) { return RIS_VERSION_PATCH; }
const char* RIS_CALL ris_get_version_string(void) { return RIS_VERSION_STRING; }

const char* RIS_CALL ris_get_last_error(void) { return t_last_error.c_str(); }
void RIS_CALL ris_clear_last_error(void) { t_last_error.clear(); }

void RIS_CALL ris_set_log_callback(ris_log_fn callback, void* user_data) {
    if (!callback) {
        ris::set_log_callback({});
        return;
    }
    ris::set_log_callback([callback, user_data](ris::LogLevel level, const char* message) {
        callback(static_cast<uint32_t>(level), message, user_data);
    });
}

void RIS_CALL ris_set_log_level(ris_log_level level) { ris::set_log_level(static_cast<ris::LogLevel>(level)); }
double RIS_CALL ris_get_time_seconds(void) { return ris::time_seconds(); }
void RIS_CALL ris_sleep_ms(uint32_t milliseconds) { ris::sleep_ms(milliseconds); }

// ---- window ---------------------------------------------------------------------------------

void RIS_CALL ris_window_desc_default(ris_window_desc* desc) {
    if (!desc) return;
    std::memset(desc, 0, sizeof(*desc));
    desc->struct_size = sizeof(ris_window_desc);
    desc->width = 1280;
    desc->height = 720;
    desc->flags = RIS_WINDOW_RESIZABLE;
    desc->backend = RIS_ANGLE_BACKEND_AUTO;
    desc->vsync = 1;
    desc->depth_bits = 24;
    desc->stencil_bits = 8;
}

ris_window* RIS_CALL ris_window_create(const ris_window_desc* desc) {
    if (!desc || desc->struct_size < sizeof(ris_window_desc)) {
        invalid("ris_window_create: desc is null or struct_size is too small (call ris_window_desc_default)");
        return nullptr;
    }
    return guard_ptr<ris_window>([&] {
        ris::WindowDesc d;
        d.title = desc->title ? desc->title : "ris-framework";
        d.width = desc->width;
        d.height = desc->height;
        d.flags = static_cast<ris::WindowFlags>(desc->flags);
        d.backend = static_cast<ris::AngleBackend>(desc->backend);
        d.vsync = desc->vsync != 0;
        d.msaa_samples = desc->msaa_samples;
        d.depth_bits = desc->depth_bits;
        d.stencil_bits = desc->stencil_bits;
        d.debug = desc->debug != 0;
        return new ris_window(d);
    });
}

void RIS_CALL ris_window_destroy(ris_window* window) { delete window; }

void RIS_CALL ris_window_swap_buffers(ris_window* window) {
    if (!window) return;
    guard([&] { window->swap_buffers(); });
}
void RIS_CALL ris_window_set_vsync(ris_window* window, uint32_t enabled) {
    if (window) guard([&] { window->set_vsync(enabled != 0); });
}
void RIS_CALL ris_window_set_title(ris_window* window, const char* title) {
    if (window && title) guard([&] { window->set_title(title); });
}
void RIS_CALL ris_window_set_fullscreen(ris_window* window, uint32_t enabled) {
    if (window) guard([&] { window->set_fullscreen(enabled != 0); });
}
void RIS_CALL ris_window_show(ris_window* window) {
    if (window) guard([&] { window->show(); });
}
void RIS_CALL ris_window_get_size(const ris_window* window, int32_t* width, int32_t* height) {
    int32_t w = 0, h = 0;
    if (window) window->size(w, h);
    if (width) *width = w;
    if (height) *height = h;
}
void RIS_CALL ris_window_get_drawable_size(const ris_window* window, int32_t* width, int32_t* height) {
    int32_t w = 0, h = 0;
    if (window) window->drawable_size(w, h);
    if (width) *width = w;
    if (height) *height = h;
}
float RIS_CALL ris_window_get_content_scale(const ris_window* window) { return window ? window->content_scale() : 1.0f; }
void RIS_CALL ris_window_set_text_input(ris_window* window, uint32_t enabled) {
    if (window) guard([&] { window->set_text_input(enabled != 0); });
}
void RIS_CALL ris_window_request_quit(ris_window* window) {
    if (window) guard([&] { window->request_quit(); });
}

const char* RIS_CALL ris_window_get_gl_info(const ris_window* window, ris_gl_info which) {
    return window ? window->gl_info(static_cast<ris::GlInfo>(which)) : "";
}
uint32_t RIS_CALL ris_window_has_gl_extension(const ris_window* window, const char* name) {
    return window && window->has_gl_extension(name) ? 1u : 0u;
}

uint32_t RIS_CALL ris_window_poll_event(ris_window* window, ris_event* out_event) {
    if (!window || !out_event) return 0;
    ris::Event ev;
    bool got = false;
    guard([&] { got = window->poll_event(ev); });
    if (!got) return 0;
    std::memcpy(out_event, &ev, sizeof(ris_event));
    return 1;
}

uint32_t RIS_CALL ris_input_key_down(int32_t scancode) { return ris::input::key_down(scancode) ? 1u : 0u; }
uint32_t RIS_CALL ris_input_mouse_state(float* x, float* y) { return ris::input::mouse_state(x, y); }

// ---- graphics state -------------------------------------------------------------------------

void RIS_CALL ris_gfx_viewport(int32_t x, int32_t y, int32_t width, int32_t height) {
    guard([&] { ris::gfx::viewport(x, y, width, height); });
}
void RIS_CALL ris_gfx_set_scissor(uint32_t enabled, int32_t x, int32_t y, int32_t width, int32_t height) {
    guard([&] { ris::gfx::set_scissor(enabled != 0, x, y, width, height); });
}
void RIS_CALL ris_gfx_clear(float r, float g, float b, float a, uint32_t clear_flags) {
    guard([&] { ris::gfx::clear(r, g, b, a, static_cast<ris::ClearFlags>(clear_flags)); });
}
void RIS_CALL ris_gfx_set_blend_mode(uint32_t blend_mode) {
    guard([&] { ris::gfx::set_blend_mode(static_cast<ris::BlendMode>(blend_mode)); });
}
void RIS_CALL ris_gfx_set_depth_state(uint32_t test_enabled, uint32_t write_enabled, uint32_t compare_func) {
    guard([&] {
        ris::gfx::set_depth_state(test_enabled != 0, write_enabled != 0, static_cast<ris::CompareFunc>(compare_func));
    });
}
void RIS_CALL ris_gfx_set_cull_mode(uint32_t cull_mode) {
    guard([&] { ris::gfx::set_cull_mode(static_cast<ris::CullMode>(cull_mode)); });
}
ris_result RIS_CALL ris_gfx_read_pixels(int32_t x, int32_t y, int32_t width, int32_t height, void* out_rgba8, size_t out_size) {
    if (!out_rgba8) return invalid("ris_gfx_read_pixels: out_rgba8 is null");
    if (width <= 0 || height <= 0) return invalid("ris_gfx_read_pixels: width and height must be positive");
    if (static_cast<uint64_t>(width) * static_cast<uint64_t>(height) * 4u > out_size) {
        return invalid("ris_gfx_read_pixels: out_size is smaller than width * height * 4");
    }
    return guard([&] {
        const std::vector<uint8_t> pixels = ris::gfx::read_pixels(x, y, width, height);
        std::memcpy(out_rgba8, pixels.data(), pixels.size());
    });
}

// ---- buffers --------------------------------------------------------------------------------

ris_buffer* RIS_CALL ris_buffer_create(uint32_t type, uint32_t usage, const void* data, size_t size) {
    return guard_ptr<ris_buffer>([&] {
        return new ris_buffer(static_cast<ris::BufferType>(type), static_cast<ris::BufferUsage>(usage), data, size);
    });
}
ris_result RIS_CALL ris_buffer_update(ris_buffer* buffer, size_t offset, const void* data, size_t size) {
    if (!buffer) return invalid("ris_buffer_update: buffer is null");
    return guard([&] { buffer->update(offset, data, size); });
}
size_t RIS_CALL ris_buffer_get_size(const ris_buffer* buffer) { return buffer ? buffer->size() : 0; }
void RIS_CALL ris_buffer_bind_base(const ris_buffer* buffer, uint32_t binding) {
    if (buffer) guard([&] { buffer->bind_base(binding); });
}
void RIS_CALL ris_buffer_destroy(ris_buffer* buffer) { delete buffer; }

// ---- shaders --------------------------------------------------------------------------------

ris_shader* RIS_CALL ris_shader_create(const char* vertex_source, const char* fragment_source) {
    if (!vertex_source || !fragment_source) {
        invalid("ris_shader_create: source is null");
        return nullptr;
    }
    return guard_ptr<ris_shader>([&] { return new ris_shader(vertex_source, fragment_source); });
}
void RIS_CALL ris_shader_use(ris_shader* shader) {
    if (shader) guard([&] { shader->use(); });
}
int32_t RIS_CALL ris_shader_get_uniform_location(const ris_shader* shader, const char* name) {
    int32_t loc = -1;
    if (shader && name) guard([&] { loc = shader->uniform_location(name); });
    return loc;
}
int32_t RIS_CALL ris_shader_get_attrib_location(const ris_shader* shader, const char* name) {
    int32_t loc = -1;
    if (shader && name) guard([&] { loc = shader->attrib_location(name); });
    return loc;
}
void RIS_CALL ris_shader_set_int(ris_shader* s, int32_t loc, int32_t v) {
    if (s) guard([&] { s->set_int(loc, v); });
}
void RIS_CALL ris_shader_set_float(ris_shader* s, int32_t loc, float v) {
    if (s) guard([&] { s->set_float(loc, v); });
}
void RIS_CALL ris_shader_set_vec2(ris_shader* s, int32_t loc, float x, float y) {
    if (s) guard([&] { s->set_vec2(loc, x, y); });
}
void RIS_CALL ris_shader_set_vec3(ris_shader* s, int32_t loc, float x, float y, float z) {
    if (s) guard([&] { s->set_vec3(loc, x, y, z); });
}
void RIS_CALL ris_shader_set_vec4(ris_shader* s, int32_t loc, float x, float y, float z, float w) {
    if (s) guard([&] { s->set_vec4(loc, x, y, z, w); });
}
void RIS_CALL ris_shader_set_mat4(ris_shader* s, int32_t loc, const float* m) {
    if (s) guard([&] { s->set_mat4(loc, m); });
}
ris_result RIS_CALL ris_shader_bind_uniform_block(ris_shader* shader, const char* block_name, uint32_t binding) {
    if (!shader || !block_name) return invalid("ris_shader_bind_uniform_block: null argument");
    return guard([&] { shader->bind_uniform_block(block_name, binding); });
}
void RIS_CALL ris_shader_destroy(ris_shader* shader) { delete shader; }

// ---- meshes ---------------------------------------------------------------------------------

void RIS_CALL ris_mesh_desc_default(ris_mesh_desc* desc) {
    if (!desc) return;
    std::memset(desc, 0, sizeof(*desc));
    desc->struct_size = sizeof(ris_mesh_desc);
}

ris_mesh* RIS_CALL ris_mesh_create(const ris_mesh_desc* desc) {
    if (desc && desc->struct_size < sizeof(ris_mesh_desc)) {
        invalid("ris_mesh_create: struct_size is too small (call ris_mesh_desc_default)");
        return nullptr;
    }
    if (!desc || !desc->attribs || desc->attrib_count == 0 || desc->attrib_count > RIS_MAX_VERTEX_ATTRIBS ||
        desc->vertex_buffer_count > RIS_MAX_VERTEX_BUFFERS) {
        invalid("ris_mesh_create: invalid mesh description");
        return nullptr;
    }
    return guard_ptr<ris_mesh>([&] {
        ris::VertexArrayDesc d;
        for (uint32_t i = 0; i < desc->attrib_count; ++i) {
            const ris_vertex_attrib& a = desc->attribs[i];
            ris::VertexAttrib va;
            va.location = a.location;
            va.components = a.components;
            va.type = static_cast<ris::AttribType>(a.type);
            va.normalized = a.normalized != 0;
            va.offset = a.offset;
            va.buffer_slot = a.buffer_slot;
            va.divisor = a.divisor;
            va.as_integer = a.as_integer != 0;
            d.attribs.push_back(va);
        }
        for (uint32_t i = 0; i < desc->vertex_buffer_count; ++i) {
            d.vertex_buffers[i] = {desc->vertex_buffers[i], desc->vertex_strides[i]};
        }
        d.index_buffer = desc->index_buffer;
        d.index_type = static_cast<ris::IndexType>(desc->index_type);
        return new ris_mesh(d);
    });
}
void RIS_CALL ris_mesh_draw(const ris_mesh* mesh, uint32_t topology, uint32_t first, uint32_t count, uint32_t instance_count) {
    if (mesh) guard([&] { mesh->draw(static_cast<ris::Topology>(topology), first, count, instance_count); });
}
void RIS_CALL ris_mesh_destroy(ris_mesh* mesh) { delete mesh; }

// ---- textures -------------------------------------------------------------------------------

ris_texture* RIS_CALL ris_texture_create_rgba8(int32_t width, int32_t height, const void* pixels, uint32_t generate_mipmaps, uint32_t srgb) {
    return guard_ptr<ris_texture>([&] {
        return new ris_texture{ris::Texture::from_rgba8(width, height, pixels, generate_mipmaps != 0, srgb != 0)};
    });
}
ris_texture* RIS_CALL ris_texture_load_image_file(const char* path, uint32_t generate_mipmaps, uint32_t srgb, uint32_t flip_vertically) {
    if (!path) {
        invalid("ris_texture_load_image_file: path is null");
        return nullptr;
    }
    return guard_ptr<ris_texture>([&] {
        return new ris_texture{ris::Texture::from_image_file(path, generate_mipmaps != 0, srgb != 0, flip_vertically != 0)};
    });
}
ris_texture* RIS_CALL ris_texture_load_ktx2_file(const char* path) {
    if (!path) {
        invalid("ris_texture_load_ktx2_file: path is null");
        return nullptr;
    }
    return guard_ptr<ris_texture>([&] { return new ris_texture{ris::Texture::from_ktx2_file(path)}; });
}
ris_texture* RIS_CALL ris_texture_load_ktx2_memory(const void* data, size_t size) {
    return guard_ptr<ris_texture>([&] { return new ris_texture{ris::Texture::from_ktx2_memory(data, size)}; });
}
void RIS_CALL ris_sampler_desc_default(ris_sampler_desc* desc) {
    if (!desc) return;
    desc->struct_size = sizeof(ris_sampler_desc);
    desc->min_filter = RIS_FILTER_LINEAR_MIPMAP_LINEAR;
    desc->mag_filter = RIS_FILTER_LINEAR;
    desc->wrap_u = RIS_WRAP_CLAMP;
    desc->wrap_v = RIS_WRAP_CLAMP;
}

ris_result RIS_CALL ris_texture_set_sampler(ris_texture* texture, const ris_sampler_desc* sampler) {
    if (!texture || !sampler) return invalid("ris_texture_set_sampler: texture or sampler is null");
    if (sampler->struct_size < sizeof(ris_sampler_desc)) {
        return invalid("ris_texture_set_sampler: struct_size is too small (call ris_sampler_desc_default)");
    }
    return guard([&] {
        ris::SamplerDesc s;
        s.min_filter = static_cast<ris::Filter>(sampler->min_filter);
        s.mag_filter = static_cast<ris::Filter>(sampler->mag_filter);
        s.wrap_u = static_cast<ris::Wrap>(sampler->wrap_u);
        s.wrap_v = static_cast<ris::Wrap>(sampler->wrap_v);
        texture->value.set_sampler(s);
    });
}
void RIS_CALL ris_texture_bind(const ris_texture* texture, uint32_t unit) {
    if (texture) guard([&] { texture->value.bind(unit); });
}
int32_t RIS_CALL ris_texture_get_width(const ris_texture* t) { return t ? t->value.width() : 0; }
int32_t RIS_CALL ris_texture_get_height(const ris_texture* t) { return t ? t->value.height() : 0; }
uint32_t RIS_CALL ris_texture_get_mip_levels(const ris_texture* t) { return t ? t->value.mip_levels() : 0; }
uint32_t RIS_CALL ris_texture_get_gl_handle(const ris_texture* t) { return t ? t->value.gl_handle() : 0; }
void RIS_CALL ris_texture_destroy(ris_texture* texture) { delete texture; }

// ---- render targets -------------------------------------------------------------------------

ris_render_target* RIS_CALL ris_render_target_create(int32_t width, int32_t height, uint32_t with_depth_stencil) {
    return guard_ptr<ris_render_target>([&] { return new ris_render_target(width, height, with_depth_stencil != 0); });
}
void RIS_CALL ris_render_target_bind(const ris_render_target* target) {
    guard([&] {
        if (target) {
            target->bind();
        } else {
            ris::RenderTarget::unbind();
        }
    });
}
ris_texture* RIS_CALL ris_render_target_get_texture(const ris_render_target* target) {
    // The colour texture lives inside the render target, and ris_texture is layout compatible
    // with ris::Texture (a struct holding exactly one), so hand out its address.
    static_assert(sizeof(ris_texture) == sizeof(ris::Texture), "ris_texture must wrap exactly one ris::Texture");
    if (!target) return nullptr;
    return reinterpret_cast<ris_texture*>(const_cast<ris::Texture*>(&target->texture()));
}
void RIS_CALL ris_render_target_destroy(ris_render_target* target) { delete target; }

// ---- sprite batch ---------------------------------------------------------------------------

ris_sprite_batch* RIS_CALL ris_sprite_batch_create(uint32_t max_sprites_per_flush) {
    return guard_ptr<ris_sprite_batch>([&] { return new ris_sprite_batch(max_sprites_per_flush); });
}
void RIS_CALL ris_sprite_batch_begin(ris_sprite_batch* batch, const float* view_projection, uint32_t blend_mode) {
    if (batch) guard([&] { batch->begin(view_projection, static_cast<ris::BlendMode>(blend_mode)); });
}
void RIS_CALL ris_sprite_batch_draw(ris_sprite_batch* batch, const ris_sprite* sprite) {
    if (!batch || !sprite) return;
    guard([&] {
        ris::Sprite s;
        std::memcpy(static_cast<void*>(&s), sprite, sizeof(ris::Sprite));
        // `texture` is a ris_texture*, which wraps a ris::Texture as its first member.
        s.texture = sprite->texture ? &sprite->texture->value : nullptr;
        batch->draw(s);
    });
}
void RIS_CALL ris_sprite_batch_draw_quad(ris_sprite_batch* batch, ris_texture* texture, float x, float y, float width, float height, float r, float g, float b, float a) {
    if (batch) guard([&] { batch->draw_quad(texture ? &texture->value : nullptr, x, y, width, height, r, g, b, a); });
}
void RIS_CALL ris_sprite_batch_end(ris_sprite_batch* batch) {
    if (batch) guard([&] { batch->end(); });
}
void RIS_CALL ris_sprite_batch_destroy(ris_sprite_batch* batch) { delete batch; }

// ---- math -----------------------------------------------------------------------------------

namespace {
void store(float* out, const ris::Mat4& m) { std::memcpy(out, glm::value_ptr(m), 16 * sizeof(float)); }
ris::Mat4 load(const float* in) {
    ris::Mat4 m;
    std::memcpy(glm::value_ptr(m), in, 16 * sizeof(float));
    return m;
}
} // namespace

void RIS_CALL ris_mat4_identity(float* out) { if (out) store(out, ris::Mat4(1.0f)); }
void RIS_CALL ris_mat4_multiply(float* out, const float* a, const float* b) { if (out && a && b) store(out, load(a) * load(b)); }
void RIS_CALL ris_mat4_translation(float* out, float x, float y, float z) { if (out) store(out, glm::translate(ris::Mat4(1.0f), ris::Vec3(x, y, z))); }
void RIS_CALL ris_mat4_scale(float* out, float x, float y, float z) { if (out) store(out, glm::scale(ris::Mat4(1.0f), ris::Vec3(x, y, z))); }
void RIS_CALL ris_mat4_rotation_x(float* out, float radians) { if (out) store(out, glm::rotate(ris::Mat4(1.0f), radians, ris::Vec3(1, 0, 0))); }
void RIS_CALL ris_mat4_rotation_y(float* out, float radians) { if (out) store(out, glm::rotate(ris::Mat4(1.0f), radians, ris::Vec3(0, 1, 0))); }
void RIS_CALL ris_mat4_rotation_z(float* out, float radians) { if (out) store(out, glm::rotate(ris::Mat4(1.0f), radians, ris::Vec3(0, 0, 1))); }
void RIS_CALL ris_mat4_ortho(float* out, float l, float r, float b, float t, float n, float f) { if (out) store(out, glm::ortho(l, r, b, t, n, f)); }
void RIS_CALL ris_mat4_perspective(float* out, float fov, float aspect, float n, float f) { if (out) store(out, glm::perspective(fov, aspect, n, f)); }
void RIS_CALL ris_mat4_look_at(float* out, float ex, float ey, float ez, float tx, float ty, float tz, float ux, float uy, float uz) {
    if (out) store(out, glm::lookAt(ris::Vec3(ex, ey, ez), ris::Vec3(tx, ty, tz), ris::Vec3(ux, uy, uz)));
}

} // extern "C"
