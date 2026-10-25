# Changelog

## 0.1.0

First release.

- C++20 API in namespace `ris` (RAII, move-only types, exceptions) with a flat C API (`ris/ris.h`) over it, built for P/Invoke.
- SDL3 window, events and input; OpenGL ES 3.0 context through EGL (ANGLE on Windows and macOS, system EGL on Linux).
- Buffers, shaders (uniforms and uniform blocks), meshes (instancing), textures, render targets, sprite batch. Math is glm: `ris::Mat4` is `glm::mat4`, a public header-only dependency (the C API keeps plain `float[16]`).
- KTX2 textures through [ris-ktx2-cpp](https://github.com/luka712/ris-ktx2-cpp): Basis Universal data is transcoded to ASTC, then ETC2, then RGBA8.
- C API review fixes from the C# binding work: `ris_gfx_read_pixels` takes `out_size`, `ris_mesh_desc` and `ris_sampler_desc` have `struct_size` (+ `ris_mesh_desc_default`, `ris_sampler_desc_default`), explicit padding fields in `ris_sprite` and `ris_mesh_desc`, log level is `uint32_t`, `ris_render_target_bind(NULL)` restores the viewport, string lifetimes documented.
- Samples: sprite_batch, unlit_mesh and blinn_phong_mesh, ported from the TypeScript and C# demos.
- Samples: ktx2_quad and ktx2_compare (KTX2 textures as ASTC, ETC2 and RGBA8).
- PNG/JPEG/etc. loading through the stb image functions in ris-ktx2.
