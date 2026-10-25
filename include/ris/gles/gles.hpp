#pragma once

// The OpenGL ES backend layer, the C++ counterpart of the ris-framework-ts `webgl` folder (named `gles`
// here, the backend is OpenGL ES 3 through ANGLE / EGL). It exposes the low level helpers the framework is
// built on: converters from the API enums to OpenGL ES constants, and utilities to create buffers,
// shaders, programs, textures, samplers, render buffers and framebuffers.
//
// The GL objects are plain `GlUint` names owned by the caller; the RAII wrappers (Buffer, Shader, Texture,
// RenderTarget, the typed buffers) are built on these utilities. No GL header is needed to use them.

#include "ris/gles/gles_types.hpp"
#include "ris/gles/rendering/vertex_attribute.hpp"
#include "ris/gles/rendering/vertex_buffer_layout.hpp"
#include "ris/gles/utilities/gles_utilities.hpp"
#include "ris/utilities/align_utilities.hpp"
