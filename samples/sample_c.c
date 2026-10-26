/* C API sample: a colour cycling clear, an indexed triangle and a few sprites.
 * Usage: ris_sample_c [--frames N] [--hidden] [--backend opengl|vulkan|null] */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ris/ris.h>

static const char* kVs =
    "#version 300 es\n"
    "layout(location = 0) in vec2 a_pos;\n"
    "layout(location = 1) in vec3 a_color;\n"
    "uniform mat4 u_mvp;\n"
    "out vec3 v_color;\n"
    "void main() { v_color = a_color; gl_Position = u_mvp * vec4(a_pos, 0.0, 1.0); }\n";

static const char* kFs =
    "#version 300 es\n"
    "precision mediump float;\n"
    "in vec3 v_color;\n"
    "out vec4 o_color;\n"
    "void main() { o_color = vec4(v_color, 1.0); }\n";

static void on_log(uint32_t level, const char* message, void* user) {
    (void)user;
    fprintf(stderr, "[ris:%d] %s\n", (int)level, message);
}

static void fail(const char* what) {
    fprintf(stderr, "%s failed: %s\n", what, ris_get_last_error());
    exit(1);
}

int main(int argc, char** argv) {
    int max_frames = -1;
    ris_window_desc desc;
    ris_window_desc_default(&desc);
    desc.title = "ris-framework C sample";
    desc.debug = 1;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--frames") && i + 1 < argc) max_frames = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--hidden")) desc.flags |= RIS_WINDOW_HIDDEN;
        else if (!strcmp(argv[i], "--backend") && i + 1 < argc) {
            const char* b = argv[++i];
            desc.backend = !strcmp(b, "opengl") ? RIS_ANGLE_BACKEND_OPENGL
                         : !strcmp(b, "vulkan") ? RIS_ANGLE_BACKEND_VULKAN
                         : !strcmp(b, "null")   ? RIS_ANGLE_BACKEND_NULL
                                                : RIS_ANGLE_BACKEND_AUTO;
        }
    }

    ris_set_log_callback(on_log, NULL);
    printf("ris-framework %s\n", ris_get_version_string());

    ris_window* window = ris_window_create(&desc);
    if (!window) fail("ris_window_create");
    printf("GL: %s | %s\n", ris_window_get_gl_info(window, RIS_GL_INFO_RENDERER), ris_window_get_gl_info(window, RIS_GL_INFO_VERSION));

    ris_shader* shader = ris_shader_create(kVs, kFs);
    if (!shader) fail("ris_shader_create");
    const int32_t u_mvp = ris_shader_get_uniform_location(shader, "u_mvp");

    const float vertices[] = {
        /* x      y      r     g     b */
         0.0f,  0.6f,  1.0f, 0.2f, 0.2f,
        -0.6f, -0.6f,  0.2f, 1.0f, 0.2f,
         0.6f, -0.6f,  0.2f, 0.4f, 1.0f,
    };
    const uint16_t indices[] = {0, 1, 2};
    ris_buffer* vb = ris_buffer_create(RIS_BUFFER_VERTEX, RIS_USAGE_STATIC, vertices, sizeof(vertices));
    ris_buffer* ib = ris_buffer_create(RIS_BUFFER_INDEX, RIS_USAGE_STATIC, indices, sizeof(indices));
    if (!vb || !ib) fail("ris_buffer_create");

    const ris_vertex_attrib attribs[2] = {
        {0, 2, RIS_ATTRIB_FLOAT, 0, 0, 0, 0, 0},
        {1, 3, RIS_ATTRIB_FLOAT, 0, 2 * sizeof(float), 0, 0, 0},
    };
    ris_mesh_desc mesh_desc;
    ris_mesh_desc_default(&mesh_desc);
    mesh_desc.attribs = attribs;
    mesh_desc.attrib_count = 2;
    mesh_desc.vertex_buffer_count = 1;
    mesh_desc.vertex_buffers[0] = vb;
    mesh_desc.vertex_strides[0] = 5 * sizeof(float);
    mesh_desc.index_buffer = ib;
    mesh_desc.index_type = RIS_INDEX_UINT16;
    ris_mesh* mesh = ris_mesh_create(&mesh_desc);
    if (!mesh) fail("ris_mesh_create");

    /* 8x8 checkerboard */
    uint8_t checker[8 * 8 * 4];
    for (int y = 0; y < 8; ++y)
        for (int x = 0; x < 8; ++x) {
            const uint8_t v = ((x + y) & 1) ? 255 : 80;
            uint8_t* p = &checker[(y * 8 + x) * 4];
            p[0] = v; p[1] = v; p[2] = 255; p[3] = 255;
        }
    ris_texture* tex = ris_texture_create_rgba8(8, 8, checker, 0, 0);
    if (!tex) fail("ris_texture_create_rgba8");
    ris_sampler_desc sampler;
    ris_sampler_desc_default(&sampler);
    sampler.min_filter = RIS_FILTER_NEAREST;
    sampler.mag_filter = RIS_FILTER_NEAREST;
    sampler.wrap_u = RIS_WRAP_REPEAT;
    sampler.wrap_v = RIS_WRAP_REPEAT;
    ris_texture_set_sampler(tex, &sampler);

    ris_sprite_batch* batch = ris_sprite_batch_create(1024);
    if (!batch) fail("ris_sprite_batch_create");

    int frame = 0;
    int running = 1;
    while (running && (max_frames < 0 || frame < max_frames)) {
        ris_event ev;
        while (ris_window_poll_event(window, &ev)) {
            if (ev.type == RIS_EVENT_QUIT) running = 0;
            if (ev.type == RIS_EVENT_KEY_DOWN && ev.scancode == RIS_SCANCODE_ESCAPE) running = 0;
            if (ev.type == RIS_EVENT_WINDOW_RESIZED) ris_gfx_viewport(0, 0, ev.width, ev.height);
        }

        const float t = (float)ris_get_time_seconds();
        int32_t w, h;
        ris_window_get_drawable_size(window, &w, &h);
        ris_gfx_viewport(0, 0, w, h);
        ris_gfx_clear(0.1f + 0.05f * sinf(t), 0.12f, 0.18f, 1.0f, RIS_CLEAR_COLOR | RIS_CLEAR_DEPTH);

        float rot[16];
        ris_mat4_rotation_z(rot, t);
        ris_shader_use(shader);
        ris_shader_set_mat4(shader, u_mvp, rot);
        ris_mesh_draw(mesh, RIS_TOPOLOGY_TRIANGLES, 0, 3, 1);

        ris_spriteBatchBegin(batch, NULL, RIS_BLEND_ALPHA);
        for (int i = 0; i < 4; ++i) {
            ris_sprite s;
            memset(&s, 0, sizeof(s));
            s.texture = tex;
            s.x = 80.0f + 90.0f * (float)i;
            s.y = 80.0f;
            s.width = s.height = 64.0f;
            s.u1 = s.v1 = 1.0f;
            s.r = s.g = s.b = s.a = 1.0f;
            s.rotation = t + (float)i;
            s.origin_x = s.origin_y = 32.0f;
            ris_sprite_batch_draw(batch, &s);
        }
        ris_spriteBatchEnd(batch);

        ris_window_swap_buffers(window);
        ++frame;
    }

    uint8_t pixel[4] = {0};
    if (ris_gfx_read_pixels(0, 0, 1, 1, pixel, sizeof(pixel)) == RIS_OK)
        printf("rendered %d frames, pixel(0,0) = %u %u %u %u\n", frame, pixel[0], pixel[1], pixel[2], pixel[3]);

    ris_sprite_batch_destroy(batch);
    ris_destroyTexture(tex);
    ris_mesh_destroy(mesh);
    ris_buffer_destroy(ib);
    ris_buffer_destroy(vb);
    ris_shader_destroy(shader);
    ris_window_destroy(window);
    return 0;
}
