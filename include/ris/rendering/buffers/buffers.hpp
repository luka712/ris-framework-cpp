#pragma once

// TS-style typed GPU buffers, mirroring the ris-framework-ts `api/rendering/buffers` folder: `IBuffer`,
// `IVertexBuffer`, `IIndexBuffer`, `IUniformBuffer`, `IBufferFactory`, `IVertexBufferLayout`,
// `BufferUsage`, `IndexBufferType`, `VertexStepMode`, `VertexFormat` and `VertexAttribute`. Each lives in
// its own header of this folder, this header includes all of them.
//
// They are thin RAII wrappers over the low level ris::Buffer of graphics.hpp, so all of its rules apply,
// see buffer.hpp.

#include "ris/rendering/buffers/buffer.hpp"
#include "ris/rendering/buffers/buffer_factory.hpp"
#include "ris/rendering/buffers/buffer_usage.hpp"
#include "ris/rendering/buffers/index_buffer.hpp"
#include "ris/rendering/buffers/index_buffer_type.hpp"
#include "ris/rendering/buffers/make_vertex_array_desc.hpp"
#include "ris/rendering/buffers/uniform_buffer.hpp"
#include "ris/rendering/buffers/vertex_attribute.hpp"
#include "ris/rendering/buffers/vertex_buffer.hpp"
#include "ris/rendering/buffers/vertex_buffer_layout.hpp"
#include "ris/rendering/buffers/vertex_format.hpp"
#include "ris/rendering/buffers/vertex_step_mode.hpp"
