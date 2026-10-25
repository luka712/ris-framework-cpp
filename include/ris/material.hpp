#pragma once

#include <memory>

#include "core.hpp"
#include "data.hpp"
#include "graphics.hpp"

namespace ris {

class Camera;
class Scene;

/// What a material needs from the scene while drawing.
struct RenderContext {
    const Camera* camera = nullptr;
    const Scene* scene = nullptr;
};

/// Base of materials. A material draws a range of a vertex array with its shader and state.
/// The vertex layout is fixed, see Geometry: position (0), uv (1), normal (2), colour (3).
class RIS_API Material {
public:
    virtual ~Material();
    virtual void draw(const VertexArray& vertex_array, uint32_t first_index, uint32_t index_count, const Mat4& model,
                      const RenderContext& context) = 0;
};

struct MaterialResources;   // shared shaders and the 1x1 white texture

struct UnlitMaterialDescriptor {
    Color diffuse_color = Color::white();
    std::shared_ptr<Texture> diffuse_texture;   ///< optional
    CullMode cull_mode = CullMode::Back;
};

/// Colour and optional texture, no lighting.
class RIS_API UnlitMaterial : public Material {
public:
    UnlitMaterial(std::shared_ptr<MaterialResources> resources, const UnlitMaterialDescriptor& d);
    ~UnlitMaterial() override;

    Color diffuse_color;
    std::shared_ptr<Texture> diffuse_texture;
    CullMode cull_mode;

    void draw(const VertexArray& vertex_array, uint32_t first_index, uint32_t index_count, const Mat4& model,
              const RenderContext& context) override;

private:
    std::shared_ptr<MaterialResources> resources_;
};

struct BlinnPhongMaterialDescriptor {
    Color ambient_color{0.1f, 0.1f, 0.1f};
    Color diffuse_color{0.8f, 0.8f, 0.8f};
    Color specular_color{0.5f, 0.5f, 0.5f};
    float shininess = 32.0f;
    std::shared_ptr<Texture> diffuse_texture;   ///< optional
    CullMode cull_mode = CullMode::Back;
};

/// Forward Blinn-Phong shading with the scene's ambient light, up to `max_directional_lights`
/// directional lights and `max_point_lights` point lights.
class RIS_API BlinnPhongMaterial : public Material {
public:
    BlinnPhongMaterial(std::shared_ptr<MaterialResources> resources, const BlinnPhongMaterialDescriptor& d);
    ~BlinnPhongMaterial() override;

    Color ambient_color, diffuse_color, specular_color;
    float shininess;
    std::shared_ptr<Texture> diffuse_texture;
    CullMode cull_mode;

    void draw(const VertexArray& vertex_array, uint32_t first_index, uint32_t index_count, const Mat4& model,
              const RenderContext& context) override;

private:
    std::shared_ptr<MaterialResources> resources_;
};

/// Creates materials, like `MaterialFactory` of ris-framework-ts. Shaders are compiled on first use
/// and shared by all materials of the factory.
class RIS_API MaterialFactory {
public:
    MaterialFactory();
    ~MaterialFactory();

    std::shared_ptr<UnlitMaterial> create_unlit_material(const UnlitMaterialDescriptor& d = {});
    std::shared_ptr<BlinnPhongMaterial> create_blinn_phong_material(const BlinnPhongMaterialDescriptor& d = {});

private:
    std::shared_ptr<MaterialResources> resources_;
};

} // namespace ris
