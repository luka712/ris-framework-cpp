#pragma once

// The OpenGL ES backend layer, the C++ counterpart of the ris-framework-ts `webgl` folder (named `gles`
// here, the backend is OpenGL ES 3 through ANGLE / EGL). It exposes the low level helpers the framework is
// built on: converters from the API enums to OpenGL ES constants, and utilities to create buffers,
// shaders, programs, textures, samplers, render buffers and framebuffers.
//
// The GL objects are plain `GlUint` names owned by the caller; the RAII wrappers (Buffer, Shader, Texture,
// RenderTarget, the Gles buffers) are built on these utilities. No GL header is needed to use them.

#include "ris/gles/buffers/buffers.hpp"
#include "ris/gles/gles_graphics_device.hpp"
#include "ris/gles/gles_graphics_device_features.hpp"
#include "ris/gles/gles_types.hpp"
#include "ris/gles/blending/gles_blend_state.hpp"
#include "ris/gles/primitive/gles_primitive_state.hpp"
#include "ris/gles/render_pass/gles_render_pass.hpp"
#include "ris/gles/sampler/gles_sampler.hpp"
#include "ris/gles/swap_chain/gles_swap_chain.hpp"
#include "ris/gles/rendering/vertex_attribute.hpp"
#include "ris/gles/texture/gles_texture_factory.hpp"
#include "ris/gles/rendering/vertex_buffer_layout.hpp"
#include "ris/gles/utilities/gles_utilities.hpp"
#include "ris/utilities/align_utilities.hpp"
