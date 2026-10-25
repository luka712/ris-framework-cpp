#pragma once

#include <cstdint>

namespace ris::gles {

/// The framebuffer attachment point of a depth and/or stencil image. Replaces the raw `GLenum`
/// (`DEPTH_ATTACHMENT`, `DEPTH_STENCIL_ATTACHMENT`, `STENCIL_ATTACHMENT`) that ris-framework-ts takes.
enum class DepthAttachmentType : uint32_t {
    Depth = 0,          ///< `GL_DEPTH_ATTACHMENT`
    DepthStencil = 1,   ///< `GL_DEPTH_STENCIL_ATTACHMENT`
    Stencil = 2         ///< `GL_STENCIL_ATTACHMENT`
};

} // namespace ris::gles
