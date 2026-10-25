#pragma once

#include <cstdint>
#include <string_view>

#include "ris/core.hpp"
#include "ris/gles/gles_types.hpp"

/// Names GL objects for graphics debuggers (RenderDoc, apitrace, ...). The native counterpart of
/// `SpectorJSUtilities.setLabel` of ris-framework-ts, which attaches `__SPECTOR_Metadata` to WebGL objects:
/// it uses `glObjectLabel` of `GL_KHR_debug` (core in OpenGL ES 3.2), and does nothing if that is not
/// available. Labels have no effect on rendering.
namespace ris::gles::debug_label_utilities {

/// The kind of GL object that gets a label.
enum class ObjectKind : uint32_t { Buffer, Shader, Program, VertexArray, Texture, Sampler, Framebuffer, Renderbuffer };

/// True if the context supports object labels (`GL_KHR_debug`).
/// @throws Error(NotInitialized) without a window.
RIS_API bool is_supported();

/// Sets the label of a GL object. Does nothing if `label` is empty, `object` is 0 or labels are not
/// supported. Never throws for GL problems, but it leaves a GL error behind if `object` is not a live
/// object: call it after the object has been used (textures and buffers must have been bound once) and
/// after the creation was checked with error_utilities::check_error.
/// @throws Error(NotInitialized) without a window.
RIS_API void set_label(ObjectKind kind, GlUint object, std::string_view label);

} // namespace ris::gles::debug_label_utilities
