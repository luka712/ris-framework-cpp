#include "ris/gles/utilities/program_utilities.hpp"

#include <string>

#include "internal.hpp"

using namespace ris::detail;

namespace ris::gles::program_utilities {

uint32_t create(uint32_t vertex_shader, uint32_t fragment_shader) {
    require_context();
    const GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    glDetachShader(program, vertex_shader);
    glDetachShader(program, fragment_shader);

    GLint ok = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
        std::string log_text(static_cast<size_t>(len > 0 ? len : 1), '\0');
        glGetProgramInfoLog(program, len, nullptr, log_text.data());
        glDeleteProgram(program);
        throw std::runtime_error(std::string("shader link error:\n") + log_text);
    }
    return program;
}

void delete_program(uint32_t& program) noexcept {
    if (program != 0 && has_context()) glDeleteProgram(program);
    program = 0;
}

} // namespace ris::gles::program_utilities
