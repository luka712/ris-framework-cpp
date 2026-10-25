#pragma once

/// ris-framework C++ API umbrella header. For the flat C ABI include <ris/ris.h> instead.
///
/// Two layers:
///  - high level, like ris-framework-ts: framework.hpp (Framework, Scene, cameras, lights, materials, content)
///  - low level: window.hpp, graphics.hpp, sprite_batch.hpp (windows, buffers, shaders, textures)
#include "camera.hpp"
#include "content.hpp"
#include "core.hpp"
#include "data.hpp"
#include "framework.hpp"
#include "graphics.hpp"
#include "input.hpp"
#include "lights.hpp"
#include "material.hpp"
#include "math.hpp"
#include "mesh.hpp"
#include "scene.hpp"
#include "sprite_batch.hpp"
#include "time.hpp"
#include "window.hpp"
