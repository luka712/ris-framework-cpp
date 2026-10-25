# Changelog

## 0.1.0

First release.

- C++20 API in namespace `ris` (RAII, move-only types, exceptions) with a flat C API (`ris/ris.h`) over it, built for P/Invoke.
- SDL3 window, events and input; OpenGL ES 3.0 context through EGL (ANGLE on Windows and macOS, system EGL on Linux).
- Buffers, shaders (uniforms and uniform blocks), meshes (instancing), textures, render targets, sprite batch. Math is glm: `ris::Mat4` is `glm::mat4`, a public header-only dependency (the C API keeps plain `float[16]`).
- KTX2 textures through [ris-ktx2-cpp](https://github.com/luka712/ris-ktx2-cpp): Basis Universal data is transcoded to ASTC, then ETC2, then RGBA8.
- C API review fixes from the C# binding work: `ris_gfx_read_pixels` takes `out_size`, `ris_mesh_desc` and `ris_sampler_desc` have `struct_size` (+ `ris_mesh_desc_default`, `ris_sampler_desc_default`), explicit padding fields in `ris_sprite` and `ris_mesh_desc`, `ris_render_target_bind(NULL)` restores the viewport, string lifetimes documented.
- High level API like ris-framework-ts: `Framework` (listeners and game loop), `Scene`, `PerspectiveCamera` / `OrthographicCamera` / `OrbitCamera` and `CameraFactory`, ambient / directional / point lights, `UnlitMaterial` and `BlinnPhongMaterial` (built-in GLSL ES shaders), `Mesh` with sub meshes, `MeshFactory`, `GeometryBuilder`, `ContentManager` (textures, KTX2, OBJ/MTL meshes), `InputManager`, `TimeManager`, `Color`, `Rect`, and `SpriteBatch::draw` / `draw_rect`.
- The low level C++ `ris::Mesh` / `MeshDesc` are now `ris::VertexArray` / `VertexArrayDesc` so the high level `Mesh` can use the name. The C API is unchanged.
- C API: `ris_texture_set_sampler` returns a `ris_result`, `ris_set_log_level` and `ris_window_get_gl_info` take the enum types, and the `struct_size` rules are documented in `ris.h`.
- Samples: sprite_batch, unlit_mesh and blinn_phong_mesh, ported from the TypeScript and C# demos.
- Samples: ktx2_quad and ktx2_compare (KTX2 textures as ASTC, ETC2 and RGBA8).
- PNG/JPEG/etc. loading through the stb image functions in ris-ktx2.
- Typed buffers like ris-framework-ts (one header per concept in `ris/rendering/buffers/`, mirroring `api/rendering/buffers` of ris-framework-ts; `ris/buffers.hpp` includes them all): `VertexBuffer`, `IndexBuffer` (16 or 32 bit, chosen from the data), `UniformBuffer` (padded to 16 bytes, `update`, `bind_base`), `BufferFactory`, `BufferUsageFlags`, `VertexBufferLayout` / `VertexAttribute` / `VertexFormat` / `VertexStepMode` and `make_vertex_array_desc`. They wrap the low level `ris::Buffer`; no C API changes. `ris::Mesh` uses them.
