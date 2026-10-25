// Port of the `blinn_phong_mesh` example of ris-framework-ts / RisFramework.Demo: the cube from an OBJ/MTL pair
// drawn with Blinn-Phong materials (Kd, Ka falling back to Kd, Ks, Ns), a pulsing ambient light, one
// directional light and two coloured point lights orbiting the cube.
#include <array>
#include <cmath>
#include <iostream>

#include "../common/obj_loader.hpp"
#include "../common/sample_app.hpp"

namespace {

const char* kVertexSource = R"(#version 300 es
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec3 a_normal;
uniform mat4 u_model;
uniform mat4 u_view_projection;
out vec3 v_position;
out vec3 v_normal;
void main() {
    vec4 world = u_model * vec4(a_position, 1.0);
    v_position = world.xyz;
    v_normal = mat3(u_model) * a_normal;   // the model matrix is a pure rotation
    gl_Position = u_view_projection * world;
}
)";

const char* kFragmentSource = R"(#version 300 es
precision highp float;

struct PointLight {
    vec3 position;
    vec3 color;        // colour * intensity
    float linear;
    float quadratic;
};

uniform vec3 u_eye;
uniform vec3 u_ambient_light;      // colour * intensity
uniform vec3 u_sun_direction;      // direction the light travels in
uniform vec3 u_sun_color;          // colour * intensity
uniform PointLight u_point_lights[2];

uniform vec3 u_ambient_color;
uniform vec3 u_diffuse_color;
uniform vec3 u_specular_color;
uniform float u_shininess;

in vec3 v_position;
in vec3 v_normal;
out vec4 o_color;

vec3 shade(vec3 n, vec3 v, vec3 l, vec3 light_color) {
    float diff = max(dot(n, l), 0.0);
    vec3 h = normalize(l + v);
    float spec = diff > 0.0 ? pow(max(dot(n, h), 0.0), u_shininess) : 0.0;
    return light_color * (u_diffuse_color * diff + u_specular_color * spec);
}

void main() {
    vec3 n = normalize(v_normal);
    vec3 v = normalize(u_eye - v_position);

    vec3 color = u_ambient_light * u_ambient_color;
    color += shade(n, v, normalize(-u_sun_direction), u_sun_color);
    for (int i = 0; i < 2; ++i) {
        vec3 to_light = u_point_lights[i].position - v_position;
        float d = length(to_light);
        float attenuation = 1.0 / (1.0 + u_point_lights[i].linear * d + u_point_lights[i].quadratic * d * d);
        color += shade(n, v, to_light / d, u_point_lights[i].color) * attenuation;
    }
    o_color = vec4(color, 1.0);
}
)";

struct PointLightData {
    const char* name;
    std::array<float, 3> color;
    float intensity;
    float linear;
    float quadratic;
    std::array<float, 3> position{0, 0, 0};
};

} // namespace

int main(int argc, char** argv) {
    try {
        sample::App app(argc, argv, "ris sample: blinn_phong_mesh");

        const sample::ObjModel model = sample::load_obj(sample::asset("blinn_phong_mesh", "cube.obj"));
        std::cout << "Loaded cube.obj: " << model.sub_meshes.size() << " submeshes, Blinn-Phong materials, ambient light.\n";

        ris::Buffer vertices(ris::BufferType::Vertex, ris::BufferUsage::Static, model.vertices.data(),
                             model.vertices.size() * sizeof(sample::ObjVertex));
        ris::Buffer indices(ris::BufferType::Index, ris::BufferUsage::Static, model.indices.data(),
                            model.indices.size() * sizeof(uint32_t));
        ris::MeshDesc desc;
        desc.attribs = {
            {.location = 0, .components = 3, .type = ris::AttribType::Float, .offset = offsetof(sample::ObjVertex, position)},
            {.location = 1, .components = 2, .type = ris::AttribType::Float, .offset = offsetof(sample::ObjVertex, uv)},
            {.location = 2, .components = 3, .type = ris::AttribType::Float, .offset = offsetof(sample::ObjVertex, normal)},
        };
        desc.vertex_buffers[0] = {&vertices, sizeof(sample::ObjVertex)};
        desc.index_buffer = &indices;
        desc.index_type = ris::IndexType::UInt32;
        ris::Mesh cube(desc);

        ris::Shader shader(kVertexSource, kFragmentSource);
        const int u_model = shader.uniform_location("u_model");
        const int u_view_projection = shader.uniform_location("u_view_projection");
        const int u_eye = shader.uniform_location("u_eye");
        const int u_ambient_light = shader.uniform_location("u_ambient_light");
        const int u_sun_direction = shader.uniform_location("u_sun_direction");
        const int u_sun_color = shader.uniform_location("u_sun_color");
        const int u_ambient_color = shader.uniform_location("u_ambient_color");
        const int u_diffuse_color = shader.uniform_location("u_diffuse_color");
        const int u_specular_color = shader.uniform_location("u_specular_color");
        const int u_shininess = shader.uniform_location("u_shininess");
        int u_point_position[2], u_point_color[2], u_point_linear[2], u_point_quadratic[2];
        for (int i = 0; i < 2; ++i) {
            const std::string p = "u_point_lights[" + std::to_string(i) + "].";
            u_point_position[i] = shader.uniform_location((p + "position").c_str());
            u_point_color[i] = shader.uniform_location((p + "color").c_str());
            u_point_linear[i] = shader.uniform_location((p + "linear").c_str());
            u_point_quadratic[i] = shader.uniform_location((p + "quadratic").c_str());
        }

        // Lights: plain data. The ambient colour is (0.4, 0.4, 0.4) and its intensity pulses.
        const std::array<float, 3> ambient_color{0.4f, 0.4f, 0.4f};
        const std::array<float, 3> sun_color{1, 1, 1};
        const float sun_intensity = 0.8f;
        const std::array<float, 3> sun_direction{-0.4f, -0.6f, -1.0f};   // travels towards the cube, away from the camera
        std::array<PointLightData, 2> point_lights{{
            {"warm", {1.0f, 0.5f, 0.2f}, 1.2f, 0.35f, 0.44f},
            {"cool", {0.2f, 0.5f, 1.0f}, 1.2f, 0.35f, 0.44f},
        }};

        constexpr float kRotationSpeed = 0.6f;   // radians per second
        constexpr float kOrbitSpeed = 1.2f;      // radians per second
        constexpr float kOrbitRadius = 1.6f;
        constexpr float kPi = 3.14159265358979f;
        const std::array<float, 3> eye{0, 0, 3};
        float angle = 0.0f, orbit_angle = 0.0f;

        while (app.begin_frame()) {
            // Slowly pulse the ambient light.
            const float ambient_intensity = 0.75f + 0.5f * std::sin(app.elapsed_time());

            // Orbit the point lights around the cube (radius 1.6), opposite each other, tilted on y.
            orbit_angle += kOrbitSpeed * app.delta_time();
            for (size_t i = 0; i < point_lights.size(); ++i) {
                const float a = orbit_angle + static_cast<float>(i) * kPi;
                point_lights[i].position = {kOrbitRadius * std::cos(a), 0.6f * std::sin(a * 2.0f), kOrbitRadius * std::sin(a)};
            }
            angle += kRotationSpeed * app.delta_time();

            const ris::Mat4 projection = glm::perspective(glm::radians(60.0f), app.aspect(), 0.1f, 100.0f);
            const ris::Mat4 view = glm::lookAt(ris::Vec3(eye[0], eye[1], eye[2]), ris::Vec3(0, 0, 0), ris::Vec3(0, 1, 0));
            const ris::Mat4 view_projection = projection * view;
            const ris::Mat4 model_matrix = glm::rotate(ris::Mat4(1.0f), angle * 0.7f, ris::Vec3(1, 0, 0)) *
                                           glm::rotate(ris::Mat4(1.0f), angle, ris::Vec3(0, 1, 0));

            ris::gfx::clear(0.392f, 0.584f, 0.929f, 1.0f);   // corn flower blue
            ris::gfx::set_depth_state(true, true);
            ris::gfx::set_cull_mode(ris::CullMode::Back);
            ris::gfx::set_blend_mode(ris::BlendMode::None);

            shader.use();
            shader.set_mat4(u_model, model_matrix);
            shader.set_mat4(u_view_projection, view_projection);
            shader.set_vec3(u_eye, eye[0], eye[1], eye[2]);
            shader.set_vec3(u_ambient_light, ambient_color[0] * ambient_intensity, ambient_color[1] * ambient_intensity,
                            ambient_color[2] * ambient_intensity);
            shader.set_vec3(u_sun_direction, sun_direction[0], sun_direction[1], sun_direction[2]);
            shader.set_vec3(u_sun_color, sun_color[0] * sun_intensity, sun_color[1] * sun_intensity, sun_color[2] * sun_intensity);
            for (int i = 0; i < 2; ++i) {
                const PointLightData& l = point_lights[static_cast<size_t>(i)];
                shader.set_vec3(u_point_position[i], l.position[0], l.position[1], l.position[2]);
                shader.set_vec3(u_point_color[i], l.color[0] * l.intensity, l.color[1] * l.intensity, l.color[2] * l.intensity);
                shader.set_float(u_point_linear[i], l.linear);
                shader.set_float(u_point_quadratic[i], l.quadratic);
            }

            // One draw per sub mesh, each with the Blinn-Phong material of its .mtl entry.
            for (const sample::ObjSubMesh& sub : model.sub_meshes) {
                const sample::ObjMaterial& m = sub.material;
                shader.set_vec3(u_ambient_color, m.ambient[0], m.ambient[1], m.ambient[2]);
                shader.set_vec3(u_diffuse_color, m.diffuse[0], m.diffuse[1], m.diffuse[2]);
                shader.set_vec3(u_specular_color, m.specular[0], m.specular[1], m.specular[2]);
                shader.set_float(u_shininess, m.shininess);
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
