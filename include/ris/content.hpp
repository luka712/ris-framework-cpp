#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "core.hpp"
#include "graphics.hpp"

// Declared by ris-ktx2 (ktx.h), the public headers only use it by pointer.
struct ktxTexture2;

namespace ris {

class Framework;
class Scene;
class Mesh;

struct Texture2DDescriptor {
    bool generate_mipmaps = true;
    bool srgb = false;              ///< the file holds sRGB encoded colours
    bool flip_vertically = false;
};

    /// The configuration to provide when loading content with the content manager.
    struct ContentConfig {
        /// Should data be kept in memory?
        /// This allows fast loading of content on repeated loads, for example, when texture needs to be reloaded.
        bool keepDataCached = false;
    };

/// Loads textures and models from files, like `IContentManager` of ris-framework-ts (synchronous here).
class RIS_API ContentManager {
public:
    explicit ContentManager(Framework& framework) : m_framework(framework) {}

    /// TODO: doc comment
    gles::GlesTexture2D* loadTexture2D(const std::string& path, const Texture2DDescriptor& d = {}, const ContentConfig& config = {});
    /// TODO: doc comment
    ktxTexture2* loadKtx2(const std::string& path,  const ContentConfig& config = {});

    /// Loads an OBJ (+ MTL) file with one sub mesh per `usemtl` range, each with an unlit material coloured by its Kd
    /// (and `map_Kd` texture when present). The mesh is added to the scene.
    Mesh& load_mesh(const std::string& path, Scene& scene);
    /// Same, with Blinn-Phong materials from Kd, Ka (falls back to Kd), Ks, Ns and `map_Kd`.
    Mesh& load_blinn_phong_mesh(const std::string& path, Scene& scene);

private:
    Framework& m_framework;
    /// Owns every texture loaded by `loadTexture2D`, they live as long as the framework.
    std::vector<std::unique_ptr<gles::GlesTexture2D>> textures_;
    std::map<std::string, gles::GlesTexture2D*> m_textureCache;
    std::map<std::string, ktxTexture2*> m_ktx2TextureCache;
};

} // namespace ris
