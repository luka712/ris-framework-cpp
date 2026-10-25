#pragma once

// Graphics API neutral GPU buffer interface, mirroring the ris-framework-ts `api/rendering/buffers` folder:
// the abstract classes `ABuffer` (`IBuffer`), `AVertexBuffer`, `AIndexBuffer`, `AUniformBuffer` and
// `ABufferFactory`, plus the plain data types `BufferUsageFlags` (`BufferUsage`), `IndexBufferType`,
// `VertexStepMode`, `VertexFormat`, `VertexAttribute` and `VertexBufferLayout`, and the shared argument
// checks of buffer_validation.hpp. Each lives in its own header of this folder, this header includes all
// of them.
//
// Nothing here depends on a graphics API. The implementations live in the backend folders: the OpenGL ES
// ones are in ris/gles/buffers/ (GlesVertexBuffer, GlesIndexBuffer, GlesUniformBuffer, GlesBufferFactory,
// like `webgl/buffers` of ris-framework-ts). Get the factory of the default backend from
// ris/rendering/default_buffer_factory.hpp. To add a backend, derive from the abstract classes in
// `ris/<api>/buffers/` and use buffer_validation.hpp for the argument checks.

#include "ris/rendering/buffers/a_buffer.hpp"
#include "ris/rendering/buffers/a_buffer_factory.hpp"
#include "ris/rendering/buffers/a_index_buffer.hpp"
#include "ris/rendering/buffers/a_uniform_buffer.hpp"
#include "ris/rendering/buffers/a_vertex_buffer.hpp"
#include "ris/rendering/buffers/buffer_usage.hpp"
#include "ris/rendering/buffers/buffer_validation.hpp"
#include "ris/rendering/buffers/index_buffer_type.hpp"
#include "ris/rendering/buffers/vertex_attribute.hpp"
#include "ris/rendering/buffers/vertex_buffer_layout.hpp"
#include "ris/rendering/buffers/vertex_format.hpp"
#include "ris/rendering/buffers/vertex_step_mode.hpp"
