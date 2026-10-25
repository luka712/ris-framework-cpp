#pragma once

/// ris-framework C++ API umbrella header. For the flat C ABI include <ris/ris.h> instead.
///
/// Two layers:
///  - high level, like ris-framework-ts: framework.hpp (Framework, Scene, cameras, lights, materials, content)
///  - low level: window.hpp, graphics.hpp, rendering/buffers/buffers.hpp, sprite_batch.hpp
///    (windows, buffers, shaders, textures)
///  - OpenGL ES backend helpers, like the `webgl` folder of ris-framework-ts: gles/gles.hpp
///    (converters and utilities for buffers, shaders, textures, framebuffers, ...)
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
#include "scene.hpp"
#include "sprite_batch.hpp"
#include "time.hpp"
#include "window.hpp"
