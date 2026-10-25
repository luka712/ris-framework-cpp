#include "ris/gles/utilities/debug_label_utilities.hpp"

#include <EGL/egl.h>

#include "internal.hpp"
#include "ris/gles/utilities/gles_utilities.hpp"

using namespace ris::detail;

namespace ris::gles::debug_label_utilities {

namespace {

using ObjectLabelFn = void (*)(GLenum identifier, GLuint name, GLsizei length, const char* label);

// Object identifiers of glObjectLabel. The ones that GL_KHR_debug adds are not in <GLES3/gl3.h>.
constexpr GLenum kBuffer = 0x82E0;
constexpr GLenum kShader = 0x82E1;
constexpr GLenum kProgram = 0x82E2;
constexpr GLenum kVertexArray = 0x8074;
constexpr GLenum kSampler = 0x82E6;

GLenum identifier(ObjectKind kind) {
    switch (kind) {
        case ObjectKind::Buffer: return kBuffer;
        case ObjectKind::Shader: return kShader;
        case ObjectKind::Program: return kProgram;
        case ObjectKind::VertexArray: return kVertexArray;
        case ObjectKind::Texture: return GL_TEXTURE;
        case ObjectKind::Sampler: return kSampler;
        case ObjectKind::Framebuffer: return GL_FRAMEBUFFER;
        case ObjectKind::Renderbuffer: return GL_RENDERBUFFER;
    }
    return 0;
}

ObjectLabelFn object_label_function() {
    // Extension entry points are looked up through EGL. The core name is used by OpenGL ES 3.2 contexts,
    // the KHR suffix by GL_KHR_debug on older ones.
    static const ObjectLabelFn fn = [] {
        void (*p)() = eglGetProcAddress("glObjectLabelKHR");
        if (!p) p = eglGetProcAddress("glObjectLabel");
        return reinterpret_cast<ObjectLabelFn>(p);
    }();
    return fn;
}

} // namespace

bool is_supported() {
    require_context();
    return GlesUtilities::context().isExtensionSupported("GL_KHR_debug") && object_label_function() != nullptr;
}

void set_label(ObjectKind kind, GlUint object, std::string_view label) {
    require_context();
    if (label.empty() || object == 0 || !is_supported()) return;
    object_label_function()(identifier(kind), object, static_cast<GLsizei>(label.size()), label.data());
}

} // namespace ris::gles::debug_label_utilities
