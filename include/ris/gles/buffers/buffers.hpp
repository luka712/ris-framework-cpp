#pragma once

// The OpenGL ES implementation of the buffer interface of ris/rendering/buffers/, like the `webgl/buffers`
// folder of ris-framework-ts: GlesVertexBuffer, GlesIndexBuffer, GlesUniformBuffer and GlesBufferFactory (all
// derived from the abstract classes of ris/rendering/buffers/), the shared GlesBufferObject, and
// make_vertex_array_desc. Each lives in its own header of this folder, this header includes all of them.

#include "ris/gles/buffers/gles_buffer_factory.hpp"
#include "ris/gles/buffers/gles_buffer_object.hpp"
#include "ris/gles/buffers/gles_index_buffer.hpp"
#include "ris/gles/buffers/gles_uniform_buffer.hpp"
#include "ris/gles/buffers/gles_vertex_buffer.hpp"
#include "ris/gles/buffers/make_vertex_array_desc.hpp"
