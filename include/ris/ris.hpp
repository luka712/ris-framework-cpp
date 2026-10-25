#pragma once

/// ris-framework C++ API umbrella header. For the flat C ABI include <ris/ris.h> instead.
///
/// Two layers:
///  - high level, like ris-framework-ts: framework.hpp (Framework, Scene, cameras, lights, materials, content)
///  - low level: window.hpp, graphics.hpp, rendering/buffers/buffers.hpp (graphics API neutral),
///    rendering/default_buffer_factory.hpp, sprite_batch.hpp
///    (windows, buffers, shaders, textures)
///  - OpenGL ES backend, like the `webgl` folder of ris-framework-ts: gles/gles.hpp (the Gles buffer classes
///    derived from the abstract buffers, converters and utilities for shaders, textures, framebuffers, ...).
///    Other rendering APIs will get their own folder next to it.
#include "camera.hpp"
#include "content.hpp"
#include "core.hpp"
#include "data.hpp"
#include "framework.hpp"
#include "gles/gles.hpp"
#include "graphics.hpp"
#include "input.hpp"
#include "lights.hpp"
#include "material.hpp"
#include "math.hpp"
#include "mesh.hpp"
#include "rendering/buffers/buffers.hpp"
#include "rendering/default_buffer_factory.hpp"
#include "scene.hpp"
#include "sprite_batch.hpp"
#include "time.hpp"
#include "window.hpp"
