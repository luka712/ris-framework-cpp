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
ASTC > ETC2 > RGBA8 transcoding), render targets, sprite batch, `Mat4`.

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

See `samples/` for a triangle plus sprites in C and C++.

## Platform notes

- Windows: ANGLE over D3D11. macOS: ANGLE over Metal. `RIS_ANGLE_BACKEND` (`d3d11`, `vulkan`, `opengl`, ...) or
  `WindowDesc::backend` picks another backend.
- Linux: the vcpkg ANGLE port builds no rendering backend on Linux, so the system EGL + GLESv2 (Mesa or the vendor
  driver) is used, via the X11 video driver. If the loaded `libEGL` is a real ANGLE build it is detected and used as ANGLE.
- One window (and one context) at a time in 0.1.0. Graphics objects must be used on the thread that created the window.

## License

MIT, see `LICENSE` and `THIRD_PARTY_NOTICES.md`.
