// Port of the `unlit_mesh` example of ris-framework-ts / RisFramework.Demo: cube.obj with one unlit material
// per face (colour from the .mtl Kd), drawn with a rotating cube and a perspective camera.
#include <cmath>
#include <iostream>

#include "../common/obj_loader.hpp"
#include "../common/sample_app.hpp"

namespace {
const char* kVertexSource = R"(#version 300 es
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec4 a_color;
uniform mat4 u_mvp;
out vec4 v_color;
void main() {
    v_color = a_color;
    gl_Position = u_mvp * vec4(a_position, 1.0);
}
)";
const char* kFragmentSource = R"(#version 300 es
precision mediump float;
in vec4 v_color;
out vec4 o_color;
void main() { o_color = v_color; }
)";
} // namespace

int main(int argc, char** argv) {
    try {
        sample::App app(argc, argv, "ris sample: unlit_mesh");

        // One sub mesh per usemtl range (6 faces -> 6 sub meshes), the loader bakes Kd into vertex colours.
        const sample::ObjModel model = sample::load_obj(sample::asset("unlit_mesh", "cube.obj"));
        std::cout << "Loaded cube.obj: " << model.sub_meshes.size() << " submeshes, unlit materials.\n";

        ris::Buffer vertices(ris::BufferType::Vertex, ris::BufferUsage::Static, model.vertices.data(),
                             model.vertices.size() * sizeof(sample::ObjVertex));
        ris::Buffer indices(ris::BufferType::Index, ris::BufferUsage::Static, model.indices.data(),
                            model.indices.size() * sizeof(uint32_t));
        ris::MeshDesc desc;
        desc.attribs = {
            {.location = 0, .components = 3, .type = ris::AttribType::Float, .offset = offsetof(sample::ObjVertex, position)},
            {.location = 1, .components = 4, .type = ris::AttribType::Float, .offset = offsetof(sample::ObjVertex, color)},
        };
        desc.vertex_buffers[0] = {&vertices, sizeof(sample::ObjVertex)};
        desc.index_buffer = &indices;
        desc.index_type = ris::IndexType::UInt32;
        ris::Mesh cube(desc);

        ris::Shader shader(kVertexSource, kFragmentSource);
        const int u_mvp = shader.uniform_location("u_mvp");

        constexpr float kRotationSpeed = 0.6f;   // radians per second
        float angle = 0.0f;

        while (app.begin_frame()) {
            angle += kRotationSpeed * app.delta_time();

            // Perspective camera at z = 3 looking at the origin, 60 degrees field of view.
            const ris::Mat4 projection = ris::Mat4::perspective(1.0471976f, app.aspect(), 0.1f, 100.0f);
            const ris::Mat4 view = ris::Mat4::look_at({0, 0, 3}, {0, 0, 0}, {0, 1, 0});
            // Euler radians (x, y), applied Y then X.
            const ris::Mat4 model_matrix = ris::Mat4::rotation_x(angle * 0.7f) * ris::Mat4::rotation_y(angle);
            const ris::Mat4 mvp = projection * view * model_matrix;

            ris::gfx::clear(1.0f, 0.714f, 0.757f, 1.0f);   // light pink
            ris::gfx::set_depth_state(true, true);
            ris::gfx::set_cull_mode(ris::CullMode::Back);
            ris::gfx::set_blend_mode(ris::BlendMode::None);

            shader.use();
            shader.set_mat4(u_mvp, mvp.data());
            for (const sample::ObjSubMesh& sub : model.sub_meshes) {
                cube.draw(ris::Topology::Triangles, sub.index_start, sub.index_count);
            }

            app.end_frame();
        }
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
