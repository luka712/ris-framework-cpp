#pragma once

/// ris-framework C++ API umbrella header. For the flat C ABI include <ris/ris.h> instead.
///
/// Two layers:
///  - high level, like ris-framework-ts: framework.hpp (Framework, Scene, cameras, lights, materials, content)
///  - low level: window.hpp, graphics.hpp, rendering/buffers/buffers.hpp (graphics API neutral),
///    rendering/default_buffer_factory.hpp, sprite_batch.hpp
///    (windows, buffers, shaders, textures), rendering/a_graphics_device.hpp (AGraphicsDevice, the GPU entry point:
///    GPU info and features, samplers, blend / primitive states, render passes, swap chains; get it from
///    Framework::graphicsDevice() or rendering/default_graphics_device.hpp), rendering/texture/a_texture2D.hpp
///    (ATexture2D and ATextureView, what the texture factory creates)
///  - OpenGL ES backend, like the `webgl` folder of ris-framework-ts: gles/gles.hpp (the Gles buffer classes
///    derived from the abstract buffers, GlesGraphicsDevice, converters and utilities for shaders, textures, framebuffers, ...).
///    Other rendering APIs will get their own folder next to it.
///  - input: input.hpp (the SDL3 based ris::InputManager, keyboard, mouse, gamepad and touch state) and time:
///    time.hpp (ris::TimeManager, GameTime, Stopwatch, Timer, ...). SDL3 is an integral part of the engine, so
///    these are concrete classes; only the rendering API is pluggable.
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
#include "rendering/a_graphics_device.hpp"
#include "rendering/default_buffer_factory.hpp"
#include "rendering/default_graphics_device.hpp"
#include "rendering/graphics_device_descriptor.hpp"
#include "rendering/texture/a_texture2D.hpp"
#include "rendering/texture/a_texture_factory.hpp"
#include "rendering/texture/a_texture_view.hpp"
#include "scene.hpp"
#include "sprites/sprite_batch.hpp"
#include "time.hpp"
#include "window.hpp"
