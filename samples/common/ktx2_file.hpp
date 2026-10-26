#pragma once
// Small helper shared by the ktx2 samples: loads a .ktx2 file through ris::gles::GlesTexture2D::from_ktx2_memory.

#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

#include <ris/ris.hpp>

namespace sample {

/// Reads the .ktx2 file and creates a texture from it (Basis Universal data is transcoded to ASTC / ETC2 / RGBA8).
inline ris::gles::GlesTexture2D load_ktx2_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("could not open " + path);
    const std::vector<char> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return ris::gles::GlesTexture2D::from_ktx2_memory(bytes.data(), bytes.size());
}

} // namespace sample
