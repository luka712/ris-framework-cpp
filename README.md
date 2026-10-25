# ris-framework-cpp

A small C++20 framework for OpenGL ES 3 applications. Window, events and input come from SDL3, the OpenGL ES context
comes from EGL (ANGLE on Windows and macOS), and KTX2 textures are loaded through
[ris-ktx2-cpp](https://github.com/luka712/ris-ktx2-cpp).

There are two APIs:

- **C++** (`#include <ris/ris.hpp>`, namespace `ris`): RAII, move-only types, errors are `ris::Error` exceptions.
  This is the primary API.
- **C** (`#include <ris/ris.h>`): a flat `extern "C"` layer over the C++ API for P/Invoke and other FFIs. Opaque
  handles, `ris_result` codes, `ris_get_last_error()` (thread local), `uint32_t` booleans, column-major `float[16]` matrices.

## Features

Window and GL context, events, buffers, shaders, meshes (instancing), textures (PNG/JPEG via stb, KTX2 with
ASTC > ETC2 > RGBA8 transcoding), render targets, sprite batch. Math is [glm](https://github.com/g-truc/glm) (`ris::Mat4` is `glm::mat4`).

The OpenGL ES helpers (buffers, shaders, programs, textures, samplers, framebuffers, blending, culling, ...) live in `ris/gles/` and mirror `webgl/` of ris-framework-ts; `ris/gles/gles.hpp` includes all of them.

## Building

Needs CMake 3.30+, a C++20 compiler and [vcpkg](https://github.com/microsoft/vcpkg).

```sh
cp CMakeUserPresets.template.json CMakeUserPresets.json   # set VCPKG_ROOT
cmake --preset debug-local
cmake --build cmake-build-debug
ctest --test-dir cmake-build-debug --output-on-failure
./cmake-build-debug/ris_sample_cpp
```

Options: `-DBUILD_SHARED_LIBS=OFF` for a static library, `-DRIS_KTX2_SOURCE_DIR=<path>` to use a local ris-ktx2-cpp
checkout (otherwise an installed `ris_ktx2` package, or a pinned commit of the `0.1.0` branch is fetched).
ris-ktx2 is always linked statically into `libris_framework`, so a shared build is one library.

## Using it

The high level API mirrors [ris-framework-ts](https://github.com/luka712/ris-framework-ts): `Framework`, `Scene`, cameras,
lights, materials, `ContentManager`, `InputManager`, `SpriteBatch::draw(texture, rect, color)`.

```cpp
#include <ris/ris.hpp>

int main() {
    ris::Framework framework({.title = "hello"});
    ris::Scene scene;   // declared after the framework: destroy scenes and textures before it
    framework.add_on_load_content_listener([&] {
        framework.camera_factory().create_perspective_camera({.aspect_ratio = 16.0f / 9.0f}, &scene);
        scene.create_directional_light({.direction = {-0.4f, -0.6f, 1}});
        framework.content().load_blinn_phong_mesh("cube.obj", scene);
    });
    framework.add_on_update_listener([&](const ris::GameTime& t) {
        scene.meshes()[0]->rotation.y += t.delta_time_sec();
        scene.camera()->update(t);
    });
    framework.add_on_render_listener([&] { scene.render(); });
    framework.renderer().clear_color = ris::Color::corner_flower_blue();
    framework.initialize();   // runs the loop until the window closes
}
```

Differences to the TS API: calls are synchronous (no `async` content loading), lights are created with
`scene.create_point_light(...)` instead of `new PointLight(..., scene)`, and the camera keeps the TS convention that the
look-at negates `eye.z`. Not ported in 0.1.0: touch and gamepad input, WebGPU specific buffer and pipeline factories.

The low level API (windows, buffers, shaders, vertex arrays, textures) stays available:

```cpp
#include <ris/ris.hpp>

int main() {
    ris::Window window({.title = "hello", .width = 800, .height = 600});
    bool running = true;
    while (running) {
        ris::Event e;
        while (window.poll_event(e)) running &= e.type != ris::EventType::Quit;
        ris::gfx::clear(0.1f, 0.2f, 0.3f, 1.0f);
        window.swap_buffers();
    }
}
```

See `samples/` for a triangle plus sprites in C and C++, and C++ ports of the examples of ris-framework-ts and
RisFramework.Demo:

| Target | What it shows |
| --- | --- |
| `ris_sample_sprite_batch` | `Framework`, `ContentManager::load_texture_2d`, `SpriteBatch::draw` |
| `ris_sample_unlit_mesh` | `Scene`, perspective camera, OBJ/MTL cube with `UnlitMaterial` per face |
| `ris_sample_blinn_phong_mesh` | `BlinnPhongMaterial`, ambient, directional and two orbiting point lights |
| `ris_sample_ktx2_quad` | loads a Basis Universal `.ktx2` (transcoded to ASTC or ETC2) and shows it as a textured quad |
| `ris_sample_ktx2_compare` | the same image as ASTC 4x4, ETC2 and RGBA8 `.ktx2` files side by side, skips formats the GPU lacks |

All samples accept `--frames N`, `--hidden`, `--size WxH` and `--screenshot out.ppm`, for example
`./cmake-build-debug/ris_sample_unlit_mesh --frames 60 --hidden --screenshot cube.ppm`. Escape closes a window.
Assets live in `samples/<name>/assets` (kodim23.png is from the Kodak image suite, see `samples/sprite_batch/README.md`; the `.ktx2` files come from the ris-ktx2-cpp test image, see `samples/ktx2_quad/README.md`).
The two ktx2 samples size their window from `--scale N` (default 2) and lay out for that size, so use `--scale` rather than `--size` with them.

## Platform notes

- Windows: ANGLE over D3D11. macOS: ANGLE over Metal. `RIS_ANGLE_BACKEND` (`d3d11`, `vulkan`, `opengl`, ...) or
  `WindowDesc::backend` picks another backend.
- Linux: the vcpkg ANGLE port builds no rendering backend on Linux, so the system EGL + GLESv2 (Mesa or the vendor
  driver) is used, via the X11 video driver. If the loaded `libEGL` is a real ANGLE build it is detected and used as ANGLE.
- One window (and one context) at a time in 0.1.0. Graphics objects must be used on the thread that created the window.

## License

MIT, see `LICENSE` and `THIRD_PARTY_NOTICES.md`.
