#pragma once
// Minimal OBJ / MTL loader for the samples. Triangulates polygons (fan), creates one sub mesh per
// `usemtl` range and one vertex per face corner. Faces must use v/vt/vn indices.

#include <array>
#include <cstdint>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace sample {

struct ObjMaterial {
    std::array<float, 3> ambient{-1, -1, -1};   ///< Ka, falls back to Kd when not given
    std::array<float, 3> diffuse{0.8f, 0.8f, 0.8f};
    std::array<float, 3> specular{0, 0, 0};
    float shininess = 32.0f;
};

struct ObjVertex {
    float position[3];
    float uv[2];
    float normal[3];
    float color[4];   ///< Kd of the material of the face, alpha 1
};

struct ObjSubMesh {
    std::string material_name;
    ObjMaterial material;
    uint32_t index_start = 0;
    uint32_t index_count = 0;
};

struct ObjModel {
    std::vector<ObjVertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<ObjSubMesh> sub_meshes;
};

inline std::map<std::string, ObjMaterial> load_mtl(const std::string& path) {
    std::map<std::string, ObjMaterial> materials;
    std::ifstream in(path);
    if (!in) return materials;   // a missing .mtl just means default materials
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
        } else if (current && key == "Kd") {
            s >> current->diffuse[0] >> current->diffuse[1] >> current->diffuse[2];
        } else if (current && key == "Ka") {
            s >> current->ambient[0] >> current->ambient[1] >> current->ambient[2];
        } else if (current && key == "Ks") {
            s >> current->specular[0] >> current->specular[1] >> current->specular[2];
        } else if (current && key == "Ns") {
            s >> current->shininess;
        }
    }
    for (auto& [name, m] : materials) {
        if (m.ambient[0] < 0) m.ambient = m.diffuse;
    }
    return materials;
}

inline ObjModel load_obj(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("could not open " + path);
    const std::string dir = path.substr(0, path.find_last_of("/\\") + 1);

    std::vector<std::array<float, 3>> positions, normals;
    std::vector<std::array<float, 2>> uvs;
    std::map<std::string, ObjMaterial> materials;
    ObjModel model;
    ObjMaterial current_material;
    std::string current_name;

    auto begin_sub_mesh = [&](const std::string& name) {
        current_name = name;
        auto it = materials.find(name);
        current_material = it != materials.end() ? it->second : ObjMaterial{};
        if (current_material.ambient[0] < 0) current_material.ambient = current_material.diffuse;
        ObjSubMesh sub;
        sub.material_name = name;
        sub.material = current_material;
        sub.index_start = static_cast<uint32_t>(model.indices.size());
        model.sub_meshes.push_back(sub);
    };

    std::string line;
    while (std::getline(in, line)) {
        std::istringstream s(line);
        std::string key;
        s >> key;
        if (key == "mtllib") {
            std::string file;
            s >> file;
            materials = load_mtl(dir + file);
        } else if (key == "v") {
            std::array<float, 3> p{};
            s >> p[0] >> p[1] >> p[2];
            positions.push_back(p);
        } else if (key == "vt") {
            std::array<float, 2> t{};
            s >> t[0] >> t[1];
            uvs.push_back(t);
        } else if (key == "vn") {
            std::array<float, 3> n{};
            s >> n[0] >> n[1] >> n[2];
            normals.push_back(n);
        } else if (key == "usemtl") {
            std::string name;
            s >> name;
            begin_sub_mesh(name);
        } else if (key == "f") {
            if (model.sub_meshes.empty()) begin_sub_mesh("");
            std::vector<uint32_t> corner;
            std::string token;
            while (s >> token) {
                int vi = 0, ti = 0, ni = 0;
                if (std::sscanf(token.c_str(), "%d/%d/%d", &vi, &ti, &ni) != 3) {
                    throw std::runtime_error("OBJ faces must use v/vt/vn: " + token);
                }
                ObjVertex v{};
                const auto& p = positions.at(static_cast<size_t>(vi - 1));
                const auto& t = uvs.at(static_cast<size_t>(ti - 1));
                const auto& n = normals.at(static_cast<size_t>(ni - 1));
                v.position[0] = p[0]; v.position[1] = p[1]; v.position[2] = p[2];
                v.uv[0] = t[0]; v.uv[1] = t[1];
                v.normal[0] = n[0]; v.normal[1] = n[1]; v.normal[2] = n[2];
                v.color[0] = current_material.diffuse[0];
                v.color[1] = current_material.diffuse[1];
                v.color[2] = current_material.diffuse[2];
                v.color[3] = 1.0f;
                corner.push_back(static_cast<uint32_t>(model.vertices.size()));
                model.vertices.push_back(v);
            }
            for (size_t i = 2; i < corner.size(); ++i) {   // triangle fan
                model.indices.push_back(corner[0]);
                model.indices.push_back(corner[i - 1]);
                model.indices.push_back(corner[i]);
            }
        }
    }
    for (ObjSubMesh& sub : model.sub_meshes) sub.index_count = 0;
    // index_count = distance to the start of the next sub mesh
    for (size_t i = 0; i < model.sub_meshes.size(); ++i) {
        const uint32_t end = i + 1 < model.sub_meshes.size() ? model.sub_meshes[i + 1].index_start
                                                             : static_cast<uint32_t>(model.indices.size());
        model.sub_meshes[i].index_count = end - model.sub_meshes[i].index_start;
    }
    return model;
}

} // namespace sample
