#pragma once

#include <memory>
#include <string>

#include "core.hpp"
#include "graphics.hpp"

namespace ris {

class AFramework;
class Scene;
class Mesh;

struct Texture2DDescriptor {
    bool generate_mipmaps = true;
    bool srgb = false;              ///< the file holds sRGB encoded colours
    bool flip_vertically = false;
};

/// Loads textures and models from files, like `IContentManager` of ris-framework-ts (synchronous here).
class RIS_API ContentManager {
public:
    explicit ContentManager(AFramework& framework) : framework_(framework) {}

    /// PNG / JPG / BMP / TGA, or KTX2 when the path ends in `.ktx2`. The framework's texture filtering preset is applied.
    std::shared_ptr<Texture> load_texture_2d(const std::string& path, const Texture2DDescriptor& d = {});
    std::shared_ptr<Texture> load_ktx2(const std::string& path);

    /// Loads an OBJ (+ MTL) file with one sub mesh per `usemtl` range, each with an unlit material coloured by its Kd
    /// (and `map_Kd` texture when present). The mesh is added to the scene.
    Mesh& load_mesh(const std::string& path, Scene& scene);
    /// Same, with Blinn-Phong materials from Kd, Ka (falls back to Kd), Ks, Ns and `map_Kd`.
    Mesh& load_blinn_phong_mesh(const std::string& path, Scene& scene);

private:
    AFramework& framework_;
};

} // namespace ris
