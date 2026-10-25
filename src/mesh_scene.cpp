#include "ris/mesh.hpp"
#include "ris/gles/buffers/make_vertex_array_desc.hpp"
#include "ris/rendering/buffers/vertex_buffer_layout.hpp"
#include "ris/rendering/default_buffer_factory.hpp"
#include "ris/scene.hpp"

#include <algorithm>
#include <cstring>

#include "internal.hpp"

using namespace ris::detail;

namespace ris {

// ---- geometry --------------------------------------------------------------------------------

Geometry GeometryBuilder::create_quad(float width, float height) {
    const float w = width * 0.5f, h = height * 0.5f;
    Geometry g;
    g.positions = {-w, -h, 0, w, -h, 0, w, h, 0, -w, h, 0};
    g.texture_coords = {0, 1, 1, 1, 1, 0, 0, 0};
    g.normals = {0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1};
    g.indices = {0, 1, 2, 0, 2, 3};
    return g;
}

Geometry GeometryBuilder::create_cube(float size) {
    const float h = size * 0.5f;
    struct Face { Vec3 n, u, v; };
    const Face faces[6] = {
        {{0, 0, 1}, {1, 0, 0}, {0, 1, 0}},   {{0, 0, -1}, {-1, 0, 0}, {0, 1, 0}},
        {{1, 0, 0}, {0, 0, -1}, {0, 1, 0}},  {{-1, 0, 0}, {0, 0, 1}, {0, 1, 0}},
        {{0, 1, 0}, {1, 0, 0}, {0, 0, -1}},  {{0, -1, 0}, {1, 0, 0}, {0, 0, 1}},
    };
    const float su[4] = {-1, 1, 1, -1}, sv[4] = {-1, -1, 1, 1};
    const float tu[4] = {0, 1, 1, 0}, tv[4] = {1, 1, 0, 0};
    Geometry g;
    for (const Face& f : faces) {
        const uint32_t base = static_cast<uint32_t>(g.vertex_count());
        for (int i = 0; i < 4; ++i) {
            const Vec3 p = (f.n + f.u * su[i] + f.v * sv[i]) * h;
            g.positions.insert(g.positions.end(), {p.x, p.y, p.z});
            g.normals.insert(g.normals.end(), {f.n.x, f.n.y, f.n.z});
            g.texture_coords.insert(g.texture_coords.end(), {tu[i], tv[i]});
        }
        g.indices.insert(g.indices.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
    }
    return g;
}

// ---- mesh ------------------------------------------------------------------------------------

namespace {
struct PackedVertex {
    float position[3];
    float uv[2];
    float normal[3];
    float color[4];
};
static_assert(sizeof(PackedVertex) == 48);
} // namespace

Mesh::Mesh(const Geometry& g, std::vector<SubMesh> sub_meshes) : sub_meshes_(std::move(sub_meshes)) {
    require_context();
    const size_t n = g.vertex_count();
    if (n == 0 || g.positions.size() % 3 != 0) {
        throw_error(Result::InvalidArgument, "Mesh: geometry needs positions (3 floats per vertex)");
    }
    if ((!g.texture_coords.empty() && g.texture_coords.size() != n * 2) || (!g.normals.empty() && g.normals.size() != n * 3) ||
        (!g.colors.empty() && g.colors.size() != n * 4)) {
        throw_error(Result::InvalidArgument, "Mesh: texture_coords / normals / colors do not match the vertex count");
    }
    std::vector<PackedVertex> vertices(n);
    for (size_t i = 0; i < n; ++i) {
        PackedVertex& v = vertices[i];
        std::memcpy(v.position, &g.positions[i * 3], sizeof(v.position));
        if (!g.texture_coords.empty()) std::memcpy(v.uv, &g.texture_coords[i * 2], sizeof(v.uv)); else v.uv[0] = v.uv[1] = 0;
        if (!g.normals.empty()) std::memcpy(v.normal, &g.normals[i * 3], sizeof(v.normal)); else { v.normal[0] = v.normal[1] = 0; v.normal[2] = 1; }
        if (!g.colors.empty()) std::memcpy(v.color, &g.colors[i * 4], sizeof(v.color)); else v.color[0] = v.color[1] = v.color[2] = v.color[3] = 1;
    }
    const std::unique_ptr<ABufferFactory> factory = create_default_buffer_factory();
    vertex_buffer_ = factory->create_vertex_buffer(
        std::span<const float>(reinterpret_cast<const float*>(vertices.data()), vertices.size() * sizeof(PackedVertex) / sizeof(float)),
        sizeof(PackedVertex));
    // position (vec3), uv (vec2), normal (vec3), colour (vec4): the locations of the built-in materials
    VertexBufferLayout layout;
    layout.array_stride = sizeof(PackedVertex);
    layout.attributes = {
        {VertexFormat::Float32x3, offsetof(PackedVertex, position), 0},
        {VertexFormat::Float32x2, offsetof(PackedVertex, uv), 1},
        {VertexFormat::Float32x3, offsetof(PackedVertex, normal), 2},
        {VertexFormat::Float32x4, offsetof(PackedVertex, color), 3},
    };
    if (!g.indices.empty()) {
        for (uint32_t index : g.indices) {
            if (index >= n) throw_error(Result::InvalidArgument, "Mesh: index %u is out of range (%zu vertices)", index, n);
        }
        index_buffer_ = factory->create_index_buffer(std::span<const uint32_t>(g.indices));
        indexed_ = true;
        draw_count_ = static_cast<uint32_t>(g.indices.size());
    } else {
        draw_count_ = static_cast<uint32_t>(n);
    }
    const VertexArrayDesc desc = gles::make_vertex_array_desc(*vertex_buffer_, layout, index_buffer_.get());
    vertex_array_ = std::make_unique<VertexArray>(desc);
    for (const SubMesh& s : sub_meshes_) {
        if (static_cast<uint64_t>(s.index_start) + s.index_count > draw_count_) {
            throw_error(Result::InvalidArgument, "Mesh: sub mesh range [%u, +%u) exceeds %u indices", s.index_start,
                        s.index_count, draw_count_);
        }
    }
}

Mesh::~Mesh() = default;

Mat4 Mesh::model_matrix() const {
    Mat4 m = glm::translate(Mat4(1.0f), position);
    m = glm::rotate(m, rotation.y, Vec3(0, 1, 0));
    m = glm::rotate(m, rotation.x, Vec3(1, 0, 0));
    m = glm::rotate(m, rotation.z, Vec3(0, 0, 1));
    return glm::scale(m, scale);
}

void Mesh::render(const RenderContext& context) const {
    if (!visible) return;
    const Mat4 model = model_matrix();
    for (const SubMesh& s : sub_meshes_) {
        if (s.material && s.index_count > 0) s.material->draw(*vertex_array_, s.index_start, s.index_count, model, context);
    }
}

Mesh& MeshFactory::create(Scene& scene, const Geometry& geometry, std::shared_ptr<Material> material) {
    const uint32_t count = static_cast<uint32_t>(geometry.indices.empty() ? geometry.vertex_count() : geometry.indices.size());
    return create_with_sub_meshes(scene, geometry, {SubMesh{std::move(material), 0, count}});
}

Mesh& MeshFactory::create_with_sub_meshes(Scene& scene, const Geometry& geometry, std::vector<SubMesh> sub_meshes) {
    return scene.add(std::make_unique<Mesh>(geometry, std::move(sub_meshes)));
}

Mesh& MeshFactory::create_quad_mesh(Scene& scene, std::shared_ptr<Material> material, float width, float height) {
    return create(scene, GeometryBuilder::create_quad(width, height), std::move(material));
}

// ---- scene -----------------------------------------------------------------------------------

Scene::Scene() {
    active_ambient_ = std::make_shared<AmbientLight>(AmbientLightDescriptor{"ambient"});
    ambient_lights_.push_back(active_ambient_);
}

Scene::~Scene() = default;

void Scene::set_active_ambient_light(std::shared_ptr<AmbientLight> light) {
    add(light);
    active_ambient_ = std::move(light);
}

namespace {
template <class T>
void add_unique(std::vector<std::shared_ptr<T>>& v, std::shared_ptr<T> item) {
    if (std::find(v.begin(), v.end(), item) == v.end()) v.push_back(std::move(item));
}
template <class T>
bool remove_by_ref(std::vector<std::shared_ptr<T>>& v, const T& item) {
    auto it = std::find_if(v.begin(), v.end(), [&](const auto& p) { return p.get() == &item; });
    if (it == v.end()) return false;
    v.erase(it);
    return true;
}
} // namespace

void Scene::add(std::shared_ptr<AmbientLight> light) { add_unique(ambient_lights_, std::move(light)); }

void Scene::add(std::shared_ptr<DirectionalLight> light) {
    if (directional_lights_.size() >= max_directional_lights &&
        std::find(directional_lights_.begin(), directional_lights_.end(), light) == directional_lights_.end()) {
        throw_error(Result::InvalidArgument, "Scene: at most %u directional lights are supported", max_directional_lights);
    }
    add_unique(directional_lights_, std::move(light));
}

void Scene::add(std::shared_ptr<PointLight> light) {
    if (point_lights_.size() >= max_point_lights &&
        std::find(point_lights_.begin(), point_lights_.end(), light) == point_lights_.end()) {
        throw_error(Result::InvalidArgument, "Scene: at most %u point lights are supported", max_point_lights);
    }
    add_unique(point_lights_, std::move(light));
}

Mesh& Scene::add(std::unique_ptr<Mesh> mesh) {
    meshes_.push_back(std::move(mesh));
    return *meshes_.back();
}

std::shared_ptr<AmbientLight> Scene::create_ambient_light(const AmbientLightDescriptor& d) {
    auto l = std::make_shared<AmbientLight>(d);
    add(l);
    return l;
}
std::shared_ptr<DirectionalLight> Scene::create_directional_light(const DirectionalLightDescriptor& d) {
    auto l = std::make_shared<DirectionalLight>(d);
    add(l);
    return l;
}
std::shared_ptr<PointLight> Scene::create_point_light(const PointLightDescriptor& d) {
    auto l = std::make_shared<PointLight>(d);
    add(l);
    return l;
}

bool Scene::remove(const AmbientLight& light) {
    if (&light == active_ambient_.get()) return false;   // the scene always has an active ambient light
    return remove_by_ref(ambient_lights_, light);
}
bool Scene::remove(const DirectionalLight& light) { return remove_by_ref(directional_lights_, light); }
bool Scene::remove(const PointLight& light) { return remove_by_ref(point_lights_, light); }
bool Scene::remove(const Mesh& mesh) {
    auto it = std::find_if(meshes_.begin(), meshes_.end(), [&](const auto& p) { return p.get() == &mesh; });
    if (it == meshes_.end()) return false;
    meshes_.erase(it);
    return true;
}

void Scene::update(const GameTime&) {}

void Scene::render() {
    if (!camera_) throw_error(Result::InvalidArgument, "Scene::render: the scene has no camera (set_camera or pass the scene to the camera factory)");
    const RenderContext context{camera_.get(), this};
    for (const auto& mesh : meshes_) mesh->render(context);
}

} // namespace ris
