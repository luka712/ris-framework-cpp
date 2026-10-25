#include "ris/content.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>

#include "internal.hpp"
#include "ris/framework.hpp"

using namespace ris::detail;

namespace ris {

namespace {

// ---- minimal OBJ / MTL parser ----------------------------------------------------------------

struct ObjMaterial {
    Vec3 ambient{-1, -1, -1};   // Ka, falls back to Kd
    Vec3 diffuse{1, 1, 1};
    std::optional<Vec3> specular;
    std::optional<float> shininess;
    float opacity = 1.0f;
    std::string diffuse_map;    // absolute or relative to the working directory
};

struct ObjSub {
    std::string material_name;
    uint32_t index_start = 0, index_count = 0;
};

struct ObjResult {
    Geometry geometry;
    std::vector<ObjSub> subs;
    std::map<std::string, ObjMaterial> materials;
};

std::string directory_of(const std::string& path) { return path.substr(0, path.find_last_of("/\\") + 1); }

std::map<std::string, ObjMaterial> parse_mtl(const std::string& path) {
    std::map<std::string, ObjMaterial> materials;
    std::ifstream in(path);
    if (!in) {
        RIS_LOG_W("could not open material library %s, using default materials", path.c_str());
        return materials;
    }
    const std::string dir = directory_of(path);
    std::string line;
    ObjMaterial* current = nullptr;
    while (std::getline(in, line)) {
        std::istringstream s(line);
        std::string key;
        s >> key;
        if (key == "newmtl") {
            std::string name;
            s >> name;
            current = &materials[name];
        } else if (!current) {
            continue;
        } else if (key == "Kd") {
            s >> current->diffuse.x >> current->diffuse.y >> current->diffuse.z;
        } else if (key == "Ka") {
            s >> current->ambient.x >> current->ambient.y >> current->ambient.z;
        } else if (key == "Ks") {
            Vec3 ks{0, 0, 0};
            s >> ks.x >> ks.y >> ks.z;
            current->specular = ks;
        } else if (key == "Ns") {
            float ns = 32;
            s >> ns;
            current->shininess = ns;
        } else if (key == "d") {
            s >> current->opacity;
        } else if (key == "Tr") {
            float tr = 0;
            s >> tr;
            current->opacity = 1.0f - tr;
        } else if (key == "map_Kd") {
            std::string file;
            std::getline(s >> std::ws, file);
            if (!file.empty()) current->diffuse_map = dir + file;
        }
    }
    for (auto& [name, m] : materials) {
        if (m.ambient.x < 0) m.ambient = m.diffuse;
    }
    return materials;
}

ObjResult parse_obj(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw_error(Result::Io, "could not open %s", path.c_str());
    const std::string dir = directory_of(path);

    std::vector<Vec3> positions, normals;
    std::vector<Vec2> uvs;
    ObjResult result;
    Geometry& g = result.geometry;

    auto begin_sub = [&](const std::string& name) {
        if (!result.subs.empty() && result.subs.back().index_count == 0 &&
            result.subs.back().index_start == g.indices.size()) {
            result.subs.pop_back();   // an empty range
        }
        ObjSub sub;
        sub.material_name = name;
        sub.index_start = static_cast<uint32_t>(g.indices.size());
        result.subs.push_back(sub);
    };

    auto resolve = [](int index, size_t count) -> long {
        if (index > 0) return index - 1;
        if (index < 0) return static_cast<long>(count) + index;
        return -1;
    };

    std::string line;
    while (std::getline(in, line)) {
        std::istringstream s(line);
        std::string key;
        s >> key;
        if (key == "mtllib") {
            std::string file;
            std::getline(s >> std::ws, file);
            for (auto& [name, m] : parse_mtl(dir + file)) result.materials[name] = std::move(m);
        } else if (key == "v") {
            Vec3 p{0, 0, 0};
            s >> p.x >> p.y >> p.z;
            positions.push_back(p);
        } else if (key == "vt") {
            Vec2 t{0, 0};
            s >> t.x >> t.y;
            uvs.push_back(t);
        } else if (key == "vn") {
            Vec3 n{0, 0, 1};
            s >> n.x >> n.y >> n.z;
            normals.push_back(n);
        } else if (key == "usemtl") {
            std::string name;
            s >> name;
            begin_sub(name);
        } else if (key == "f") {
            if (result.subs.empty()) begin_sub("");
            struct Corner { long v, t, n; };
            std::vector<Corner> corners;
            std::string token;
            while (s >> token) {
                int vi = 0, ti = 0, ni = 0;
                bool has_t = false, has_n = false;
                if (std::sscanf(token.c_str(), "%d/%d/%d", &vi, &ti, &ni) == 3) {
                    has_t = has_n = true;
                } else if (std::sscanf(token.c_str(), "%d//%d", &vi, &ni) == 2) {
                    has_n = true;
                } else if (std::sscanf(token.c_str(), "%d/%d", &vi, &ti) == 2) {
                    has_t = true;
                } else if (std::sscanf(token.c_str(), "%d", &vi) != 1) {
                    throw_error(Result::Io, "%s: invalid face element '%s'", path.c_str(), token.c_str());
                }
                const long v = resolve(vi, positions.size());
                const long t = has_t ? resolve(ti, uvs.size()) : -1;
                const long n = has_n ? resolve(ni, normals.size()) : -1;
                if (v < 0 || v >= static_cast<long>(positions.size()) || (has_t && (t < 0 || t >= static_cast<long>(uvs.size()))) ||
                    (has_n && (n < 0 || n >= static_cast<long>(normals.size())))) {
                    throw_error(Result::Io, "%s: face index out of range in '%s'", path.c_str(), token.c_str());
                }
                corners.push_back({v, t, n});
            }
            if (corners.size() < 3) continue;
            Vec3 flat{0, 0, 1};
            {
                const Vec3 e1 = positions[static_cast<size_t>(corners[1].v)] - positions[static_cast<size_t>(corners[0].v)];
                const Vec3 e2 = positions[static_cast<size_t>(corners[2].v)] - positions[static_cast<size_t>(corners[0].v)];
                const Vec3 c = glm::cross(e1, e2);
                if (glm::length(c) > 0) flat = glm::normalize(c);
            }
            const uint32_t base = static_cast<uint32_t>(g.vertex_count());
            for (const Corner& c : corners) {
                const Vec3& p = positions[static_cast<size_t>(c.v)];
                const Vec2 t = c.t >= 0 ? uvs[static_cast<size_t>(c.t)] : Vec2(0, 0);
                const Vec3 n = c.n >= 0 ? normals[static_cast<size_t>(c.n)] : flat;
                g.positions.insert(g.positions.end(), {p.x, p.y, p.z});
                g.texture_coords.insert(g.texture_coords.end(), {t.x, t.y});
                g.normals.insert(g.normals.end(), {n.x, n.y, n.z});
                g.colors.insert(g.colors.end(), {1.0f, 1.0f, 1.0f, 1.0f});   // the material carries the colour
            }
            for (uint32_t i = 2; i < corners.size(); ++i) {   // triangle fan
                g.indices.insert(g.indices.end(), {base, base + i - 1, base + i});
            }
        }
    }
    if (g.positions.empty()) throw_error(Result::Io, "%s: no geometry found", path.c_str());
    for (size_t i = 0; i < result.subs.size(); ++i) {
        const uint32_t end = i + 1 < result.subs.size() ? result.subs[i + 1].index_start : static_cast<uint32_t>(g.indices.size());
        result.subs[i].index_count = end - result.subs[i].index_start;
    }
    return result;
}

bool ends_with_ci(const std::string& s, const std::string& suffix) {
    if (s.size() < suffix.size()) return false;
    for (size_t i = 0; i < suffix.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(s[s.size() - suffix.size() + i])) != suffix[i]) return false;
    }
    return true;
}

} // namespace

std::shared_ptr<Texture> ContentManager::load_texture_2d(const std::string& path, const Texture2DDescriptor& d) {
    std::shared_ptr<Texture> texture;
    if (ends_with_ci(path, ".ktx2")) {
        texture = std::make_shared<Texture>(Texture::from_ktx2_file(path));
    } else {
        texture = std::make_shared<Texture>(Texture::from_image_file(path, d.generate_mipmaps, d.srgb, d.flip_vertically));
    }
    SamplerDesc sampler;   // clamped
    const bool has_mips = texture->mip_levels() > 1;
    switch (framework_.configuration().texture_filtering) {
    case TextureFilteringPreset::Nearest:
        sampler.min_filter = sampler.mag_filter = Filter::Nearest;
        break;
    case TextureFilteringPreset::Bilinear:
        sampler.min_filter = sampler.mag_filter = Filter::Linear;
        break;
    case TextureFilteringPreset::Trilinear:
        sampler.min_filter = has_mips ? Filter::LinearMipmapLinear : Filter::Linear;
        sampler.mag_filter = Filter::Linear;
        break;
    }
    texture->set_sampler(sampler);
    return texture;
}

std::shared_ptr<Texture> ContentManager::load_ktx2(const std::string& path) {
    return std::make_shared<Texture>(Texture::from_ktx2_file(path));
}

namespace {

std::shared_ptr<Texture> load_repeating(ContentManager& content, const std::string& path, bool srgb) {
    Texture2DDescriptor td;
    td.srgb = srgb;
    auto t = content.load_texture_2d(path, td);
    return t;
}

} // namespace

Mesh& ContentManager::load_mesh(const std::string& path, Scene& scene) {
    ObjResult obj = parse_obj(path);
    std::map<std::string, std::shared_ptr<Material>> cache;
    std::vector<SubMesh> subs;
    for (const ObjSub& s : obj.subs) {
        auto& slot = cache[s.material_name];
        if (!slot) {
            UnlitMaterialDescriptor md;
            auto it = obj.materials.find(s.material_name);
            if (it != obj.materials.end()) {
                md.diffuse_color = Color(it->second.diffuse.x, it->second.diffuse.y, it->second.diffuse.z, it->second.opacity);
                if (!it->second.diffuse_map.empty()) md.diffuse_texture = load_repeating(*this, it->second.diffuse_map, false);
            }
            slot = framework_.materialFactory().create_unlit_material(md);
        }
        subs.push_back({slot, s.index_start, s.index_count});
    }
    return framework_.meshFactory().create_with_sub_meshes(scene, obj.geometry, std::move(subs));
}

Mesh& ContentManager::load_blinn_phong_mesh(const std::string& path, Scene& scene) {
    ObjResult obj = parse_obj(path);
    std::map<std::string, std::shared_ptr<Material>> cache;
    std::vector<SubMesh> subs;
    for (const ObjSub& s : obj.subs) {
        auto& slot = cache[s.material_name];
        if (!slot) {
            BlinnPhongMaterialDescriptor md;
            auto it = obj.materials.find(s.material_name);
            if (it != obj.materials.end()) {
                const ObjMaterial& m = it->second;
                md.diffuse_color = Color(m.diffuse.x, m.diffuse.y, m.diffuse.z, m.opacity);
                md.ambient_color = Color(m.ambient.x, m.ambient.y, m.ambient.z);
                if (m.specular) md.specular_color = Color(m.specular->x, m.specular->y, m.specular->z);
                if (m.shininess) md.shininess = *m.shininess;
                if (!m.diffuse_map.empty()) md.diffuse_texture = load_repeating(*this, m.diffuse_map, false);
            }
            slot = framework_.materialFactory().create_blinn_phong_material(md);
        }
        subs.push_back({slot, s.index_start, s.index_count});
    }
    return framework_.meshFactory().create_with_sub_meshes(scene, obj.geometry, std::move(subs));
}

} // namespace ris
