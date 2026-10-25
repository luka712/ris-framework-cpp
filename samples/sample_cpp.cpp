// C++ API sample: the same scene as sample_c.c using RAII types and exceptions.
// Usage: ris_sample_cpp [--frames N] [--hidden]
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <ris/ris.hpp>

namespace {
const char* kVs = R"(#version 300 es
layout(location = 0) in vec2 a_pos;
layout(location = 1) in vec3 a_color;
uniform mat4 u_mvp;
out vec3 v_color;
void main() { v_color = a_color; gl_Position = u_mvp * vec4(a_pos, 0.0, 1.0); }
)";
const char* kFs = R"(#version 300 es
precision mediump float;
in vec3 v_color;
out vec4 o_color;
void main() { o_color = vec4(v_color, 1.0); }
)";
} // namespace

int main(int argc, char** argv) {
    try {
        int max_frames = -1;
        ris::WindowDesc desc;
        desc.title = "ris-framework C++ sample";
        for (int i = 1; i < argc; ++i) {
            if (!std::strcmp(argv[i], "--frames") && i + 1 < argc) max_frames = std::atoi(argv[++i]);
            else if (!std::strcmp(argv[i], "--hidden")) desc.flags = desc.flags | ris::WindowFlags::Hidden;
        }

        ris::Window window(desc);
        std::cout << "ris-framework " << ris::version_string() << " on "
                  << window.gl_info(ris::GlInfo::Renderer) << "\n";

        ris::Shader shader(kVs, kFs);
        const int u_mvp = shader.uniform_location("u_mvp");

        const float vertices[] = {0.0f, 0.6f, 1.0f, 0.2f, 0.2f, -0.6f, -0.6f, 0.2f, 1.0f, 0.2f, 0.6f, -0.6f, 0.2f, 0.4f, 1.0f};
        ris::Buffer vb(ris::BufferType::Vertex, ris::BufferUsage::Static, vertices, sizeof(vertices));

        ris::VertexArrayDesc md;
        md.attribs = {{0, 2, ris::AttribType::Float, false, 0, 0, 0, false},
                      {1, 3, ris::AttribType::Float, false, 2 * sizeof(float), 0, 0, false}};
        md.vertex_buffers[0] = {&vb, 5 * sizeof(float)};
        ris::VertexArray mesh(md);

        ris::SpriteBatch batch;

        int frame = 0;
        bool running = true;
        while (running && (max_frames < 0 || frame < max_frames)) {
            ris::Event ev;
            while (window.poll_event(ev)) {
                if (ev.type == ris::EventType::Quit) running = false;
            }
            const auto t = static_cast<float>(ris::time_seconds());
            int32_t w, h;
            window.drawable_size(w, h);
            ris::gfx::viewport(0, 0, w, h);
            ris::gfx::clear(0.1f, 0.12f + 0.05f * std::sin(t), 0.18f, 1.0f);

            shader.use();
            shader.set_mat4(u_mvp, glm::rotate(ris::Mat4(1.0f), t, ris::Vec3(0, 0, 1)));
            mesh.draw(ris::Topology::Triangles, 0, 3);

            batch.begin();
            batch.draw_quad(nullptr, 20.0f, 20.0f, 60.0f, 60.0f, 1.0f, 0.8f, 0.2f, 1.0f);
            batch.end();

            window.swap_buffers();
            ++frame;
        }
        std::cout << "rendered " << frame << " frames\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "ris error: " << e.what() << "\n";
        return 1;
    }
}
