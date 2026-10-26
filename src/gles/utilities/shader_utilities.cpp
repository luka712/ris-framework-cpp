#include "ris/gles/utilities/shader_utilities.hpp"

#include "internal.hpp"

using namespace ris::detail;

namespace ris::gles::shader_utilities {

uint32_t create(const std::string& source, ShaderStage stage) {
    require_context();
    GLenum type = 0;
    const char* stage_name = nullptr;
    switch (stage) {
        case ShaderStage::Vertex: type = GL_VERTEX_SHADER; stage_name = "vertex"; break;
        case ShaderStage::Fragment: type = GL_FRAGMENT_SHADER; stage_name = "fragment"; break;
        default:
            throw std::domain_error(std::string("shader stage ") + std::to_string(static_cast<unsigned>(stage))
                                    + " cannot be compiled with OpenGL ES 3.0");
    }

    const GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
        std::string log_text(static_cast<size_t>(len > 0 ? len : 1), '\0');
        glGetShaderInfoLog(shader, len, nullptr, log_text.data());
        glDeleteShader(shader);
        throw std::runtime_error(std::string(stage_name) + " shader compile error:\n" + log_text);
    }
    return shader;
}

uint32_t create_vertex_shader(const std::string& source) { return create(source, ShaderStage::Vertex); }

uint32_t create_fragment_shader(const std::string& source) { return create(source, ShaderStage::Fragment); }

void delete_shader(uint32_t& shader) noexcept {
    if (shader != 0 && has_context()) glDeleteShader(shader);
    shader = 0;
}

} // namespace ris::gles::shader_utilities
