#pragma once

#include <memory>
#include <vector>

#include "core.hpp"
#include "data.hpp"
#include "graphics.hpp"
#include "material.hpp"

namespace ris {

class Scene;

/// CPU side vertex data. Missing attributes get defaults when the mesh is created: uv (0, 0),
/// normal (0, 0, 1), colour white. `positions` is required and `indices` may be empty (non indexed).
struct Geometry {
    std::vector<float> positions;       ///< x, y, z per vertex
    std::vector<float> texture_coords;  ///< u, v per vertex
    std::vector<float> normals;         ///< x, y, z per vertex
    std::vector<float> colors;          ///< r, g, b, a per vertex
    std::vector<uint32_t> indices;

    size_t vertex_count() const { return positions.size() / 3; }
};

struct RIS_API GeometryBuilder {
    /// Quad in the xy plane facing +z, centred on the origin.
    static Geometry create_quad(float width = 1.0f, float height = 1.0f);
    /// Cube centred on the origin, with normals and uv per face.
    static Geometry create_cube(float size = 1.0f);
};

struct SubMesh {
    std::shared_ptr<Material> material;
    uint32_t index_start = 0;
    uint32_t index_count = 0;
};

/// A drawable: uploaded geometry, one or more sub meshes with their own material, and a transform.
/// Like `IMesh` of ris-framework-ts. The model matrix is T * Ry * Rx * Rz * S.
class RIS_API Mesh {
public:
    Mesh(const Geometry& geometry, std::vector<SubMesh> sub_meshes);
    ~Mesh();
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    Vec3 position{0, 0, 0};
    Vec3 rotation{0, 0, 0};   ///< Euler radians (x, y, z), applied Y then X then Z
    Vec3 scale{1, 1, 1};
    bool visible = true;

    std::vector<SubMesh>& sub_meshes() { return sub_meshes_; }
    const std::vector<SubMesh>& sub_meshes() const { return sub_meshes_; }
    /// Material of the first sub mesh, or null.
    std::shared_ptr<Material> material() const { return sub_meshes_.empty() ? nullptr : sub_meshes_[0].material; }

    Mat4 model_matrix() const;
    void render(const RenderContext& context) const;

private:
    std::unique_ptr<Buffer> vertex_buffer_, index_buffer_;
    std::unique_ptr<VertexArray> vertex_array_;
    std::vector<SubMesh> sub_meshes_;
    uint32_t draw_count_ = 0;
    bool indexed_ = false;
};

/// Creates meshes and adds them to a scene, like `MeshFactory` of ris-framework-ts.
class RIS_API MeshFactory {
public:
    /// One sub mesh covering all of the geometry.
    Mesh& create(Scene& scene, const Geometry& geometry, std::shared_ptr<Material> material);
    Mesh& create_with_sub_meshes(Scene& scene, const Geometry& geometry, std::vector<SubMesh> sub_meshes);
    /// Quad of the given size in the xy plane.
    Mesh& create_quad_mesh(Scene& scene, std::shared_ptr<Material> material, float width = 1.0f, float height = 1.0f);
};

} // namespace ris
