#include "ris/material.hpp"

#include <algorithm>
#include <optional>
#include <string>

#include "internal.hpp"
#include "ris/camera.hpp"
#include "ris/lights.hpp"
#include "ris/scene.hpp"

using namespace ris::detail;

namespace ris {

namespace {

const char* kUnlitVertex = R"(#version 300 es
precision highp float;
uniform mat4 u_mvp;
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_uv;
layout(location = 3) in vec4 a_color;
out vec2 v_uv;
out vec4 v_color;
void main() {
    v_uv = a_uv;
    v_color = a_color;
    gl_Position = u_mvp * vec4(a_position, 1.0);
}
)";

const char* kUnlitFragment = R"(#version 300 es
precision mediump float;
uniform sampler2D u_texture;
uniform vec4 u_color;
in vec2 v_uv;
in vec4 v_color;
out vec4 o_color;
void main() {
    o_color = texture(u_texture, v_uv) * v_color * u_color;
}
)";

const char* kBlinnPhongVertex = R"(#version 300 es
precision highp float;
uniform mat4 u_model;
uniform mat4 u_view_projection;
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec3 a_normal;
layout(location = 3) in vec4 a_color;
out vec2 v_uv;
out vec4 v_color;
out vec3 v_normal;
out vec3 v_world_position;
void main() {
    vec4 world = u_model * vec4(a_position, 1.0);
    v_world_position = world.xyz;
    v_normal = transpose(inverse(mat3(u_model))) * a_normal;
    v_uv = a_uv;
    v_color = a_color;
    gl_Position = u_view_projection * world;
}
)";

const char* kBlinnPhongFragment = R"(#version 300 es
precision highp float;

#define MAX_DIRECTIONAL_LIGHTS 3
#define MAX_POINT_LIGHTS 5

struct DirectionalLight {
    vec3 light;       // colour * intensity
    vec3 direction;   // the direction the light travels in
};

struct PointLight {
    vec3 light;       // colour * intensity
    vec3 position;
    float constant;
    float linear;
    float quadratic;
};

uniform vec3 u_ambient_color;
uniform vec3 u_diffuse_color;
uniform vec3 u_specular_color;
uniform float u_shininess;
uniform float u_alpha;
uniform vec3 u_camera_position;
uniform vec3 u_ambient_light;
uniform int u_directional_count;
uniform int u_point_count;
uniform DirectionalLight u_directional[MAX_DIRECTIONAL_LIGHTS];
uniform PointLight u_point[MAX_POINT_LIGHTS];
uniform sampler2D u_texture;

in vec2 v_uv;
in vec4 v_color;
in vec3 v_normal;
in vec3 v_world_position;
out vec4 o_color;

void accumulate(vec3 normal, vec3 view_direction, vec3 to_light, vec3 light_color, inout vec3 diffuse, inout vec3 specular) {
    float n_dot_l = max(dot(normal, to_light), 0.0);
    diffuse += u_diffuse_color * light_color * n_dot_l;
    if (n_dot_l > 0.0) {
        vec3 halfway = normalize(to_light + view_direction);
        float n_dot_h = max(dot(normal, halfway), 0.0);
        specular += u_specular_color * light_color * pow(n_dot_h, u_shininess);
    }
}

void main() {
    vec3 normal = normalize(v_normal);
    vec3 view_direction = normalize(u_camera_position - v_world_position);
    vec3 ambient = u_ambient_color * u_ambient_light;
    vec3 diffuse = vec3(0.0);
    vec3 specular = vec3(0.0);

    for (int i = 0; i < u_directional_count; ++i) {
        accumulate(normal, view_direction, normalize(-u_directional[i].direction), u_directional[i].light, diffuse, specular);
    }
    for (int i = 0; i < u_point_count; ++i) {
        vec3 to_light = u_point[i].position - v_world_position;
        float d = length(to_light);
        float attenuation = 1.0 / (u_point[i].constant + u_point[i].linear * d + u_point[i].quadratic * d * d);
        accumulate(normal, view_direction, to_light / max(d, 0.0001), u_point[i].light * attenuation, diffuse, specular);
    }

    vec4 texel = texture(u_texture, v_uv) * v_color;
    // Specular highlights are not tinted by the texture.
    o_color = vec4(texel.rgb * (ambient + diffuse) + specular, texel.a * u_alpha);
}
)";

} // namespace

struct MaterialResources {
    struct Unlit {
        Shader shader{kUnlitVertex, kUnlitFragment};
        int32_t mvp = shader.uniform_location("u_mvp");
        int32_t color = shader.uniform_location("u_color");
        int32_t texture = shader.uniform_location("u_texture");
    };
    struct Blinn {
        Shader shader{kBlinnPhongVertex, kBlinnPhongFragment};
        int32_t model = shader.uniform_location("u_model");
        int32_t view_projection = shader.uniform_location("u_view_projection");
        int32_t ambient_color = shader.uniform_location("u_ambient_color");
        int32_t diffuse_color = shader.uniform_location("u_diffuse_color");
        int32_t specular_color = shader.uniform_location("u_specular_color");
        int32_t shininess = shader.uniform_location("u_shininess");
        int32_t alpha = shader.uniform_location("u_alpha");
        int32_t camera_position = shader.uniform_location("u_camera_position");
        int32_t ambient_light = shader.uniform_location("u_ambient_light");
        int32_t directional_count = shader.uniform_location("u_directional_count");
        int32_t point_count = shader.uniform_location("u_point_count");
        int32_t texture = shader.uniform_location("u_texture");
        struct Dir { int32_t light, direction; } dir[max_directional_lights];
        struct Pt { int32_t light, position, constant, linear, quadratic; } pt[max_point_lights];

        Blinn() {
            for (uint32_t i = 0; i < max_directional_lights; ++i) {
                const std::string p = "u_directional[" + std::to_string(i) + "].";
                dir[i] = {shader.uniform_location((p + "light").c_str()), shader.uniform_location((p + "direction").c_str())};
            }
            for (uint32_t i = 0; i < max_point_lights; ++i) {
                const std::string p = "u_point[" + std::to_string(i) + "].";
                pt[i] = {shader.uniform_location((p + "light").c_str()), shader.uniform_location((p + "position").c_str()),
                         shader.uniform_location((p + "constant").c_str()), shader.uniform_location((p + "linear").c_str()),
                         shader.uniform_location((p + "quadratic").c_str())};
            }
        }
    };

    std::optional<Unlit> unlit;
    std::optional<Blinn> blinn;
    std::optional<Texture> white;

    Unlit& get_unlit() {
        if (!unlit) unlit.emplace();
        return *unlit;
    }
    Blinn& get_blinn() {
        if (!blinn) blinn.emplace();
        return *blinn;
    }
    const Texture& get_white() {
        if (!white) {
            const uint8_t px[4] = {255, 255, 255, 255};
            white.emplace(Texture::from_rgba8(1, 1, px, false, false));
        }
        return *white;
    }
};

Material::~Material() = default;

namespace {
void require_camera(const RenderContext& c, const char* who) {
    if (!c.camera) throw_error(Result::InvalidArgument, "%s: the scene has no camera", who);
}
} // namespace

UnlitMaterial::UnlitMaterial(std::shared_ptr<MaterialResources> resources, const UnlitMaterialDescriptor& d)
    : diffuse_color(d.diffuse_color),
      diffuse_texture(d.diffuse_texture),
      cull_mode(d.cull_mode),
      resources_(std::move(resources)) {}
UnlitMaterial::~UnlitMaterial() = default;

void UnlitMaterial::draw(const VertexArray& va, uint32_t first_index, uint32_t index_count, const Mat4& model,
                         const RenderContext& context) {
    require_camera(context, "UnlitMaterial");
    auto& r = resources_->get_unlit();
    gfx::set_blend_mode(BlendMode::Alpha);
    gfx::set_depth_state(true, true);
    gfx::set_cull_mode(cull_mode);
    r.shader.use();
    r.shader.set_mat4(r.mvp, context.camera->projection_view_matrix() * model);
    r.shader.set_vec4(r.color, diffuse_color.rgba());
    r.shader.set_int(r.texture, 0);
    (diffuse_texture ? *diffuse_texture : resources_->get_white()).bind(0);
    va.draw(Topology::Triangles, first_index, index_count);
}

BlinnPhongMaterial::BlinnPhongMaterial(std::shared_ptr<MaterialResources> resources, const BlinnPhongMaterialDescriptor& d)
    : ambient_color(d.ambient_color),
      diffuse_color(d.diffuse_color),
      specular_color(d.specular_color),
      shininess(d.shininess),
      diffuse_texture(d.diffuse_texture),
      cull_mode(d.cull_mode),
      resources_(std::move(resources)) {}
BlinnPhongMaterial::~BlinnPhongMaterial() = default;

void BlinnPhongMaterial::draw(const VertexArray& va, uint32_t first_index, uint32_t index_count, const Mat4& model,
                              const RenderContext& context) {
    require_camera(context, "BlinnPhongMaterial");
    auto& r = resources_->get_blinn();
    gfx::set_blend_mode(BlendMode::Alpha);
    gfx::set_depth_state(true, true);
    gfx::set_cull_mode(cull_mode);
    r.shader.use();
    r.shader.set_mat4(r.model, model);
    r.shader.set_mat4(r.view_projection, context.camera->projection_view_matrix());
    r.shader.set_vec3(r.ambient_color, ambient_color.rgb());
    r.shader.set_vec3(r.diffuse_color, diffuse_color.rgb());
    r.shader.set_vec3(r.specular_color, specular_color.rgb());
    r.shader.set_float(r.shininess, shininess);
    r.shader.set_float(r.alpha, diffuse_color.a);
    r.shader.set_vec3(r.camera_position, context.camera->world_position());

    Vec3 ambient{0, 0, 0};
    int32_t dir_count = 0, point_count = 0;
    if (context.scene) {
        const AmbientLight& a = context.scene->active_ambient_light();
        ambient = a.color.rgb() * a.intensity;
        const auto& dirs = context.scene->directional_lights();
        dir_count = static_cast<int32_t>(std::min<size_t>(dirs.size(), max_directional_lights));
        for (int32_t i = 0; i < dir_count; ++i) {
            const DirectionalLight& l = *dirs[static_cast<size_t>(i)];
            r.shader.set_vec3(r.dir[i].light, l.color.rgb() * l.intensity);
            r.shader.set_vec3(r.dir[i].direction, l.direction);
        }
        const auto& pts = context.scene->point_lights();
        point_count = static_cast<int32_t>(std::min<size_t>(pts.size(), max_point_lights));
        for (int32_t i = 0; i < point_count; ++i) {
            const PointLight& l = *pts[static_cast<size_t>(i)];
            r.shader.set_vec3(r.pt[i].light, l.color.rgb() * l.intensity);
            r.shader.set_vec3(r.pt[i].position, l.position);
            r.shader.set_float(r.pt[i].constant, l.constant);
            r.shader.set_float(r.pt[i].linear, l.linear);
            r.shader.set_float(r.pt[i].quadratic, l.quadratic);
        }
    }
    r.shader.set_vec3(r.ambient_light, ambient);
    r.shader.set_int(r.directional_count, dir_count);
    r.shader.set_int(r.point_count, point_count);
    r.shader.set_int(r.texture, 0);
    (diffuse_texture ? *diffuse_texture : resources_->get_white()).bind(0);
    va.draw(Topology::Triangles, first_index, index_count);
}

MaterialFactory::MaterialFactory() : resources_(std::make_shared<MaterialResources>()) {}
MaterialFactory::~MaterialFactory() = default;

std::shared_ptr<UnlitMaterial> MaterialFactory::create_unlit_material(const UnlitMaterialDescriptor& d) {
    return std::make_shared<UnlitMaterial>(resources_, d);
}

std::shared_ptr<BlinnPhongMaterial> MaterialFactory::create_blinn_phong_material(const BlinnPhongMaterialDescriptor& d) {
    return std::make_shared<BlinnPhongMaterial>(resources_, d);
}

} // namespace ris
