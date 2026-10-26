# ris API naming guide

## C++

- Types are PascalCase with no prefix, and they live in the `ris` namespace, for example `ris::Buffer` and `ris::Result`.
- Abstract classes start with `A` (`ris::ABuffer`). Backend implementations start with the backend name (`ris::gles::GlesBuffer`) or (`ris::wgpu::WGPUBuffer`).
- Methods are camelCase (`addOnLoadContentListener`).
- No type aliases. Write the real type, such as `glm::vec2`.
- Keep it simple (KISS): plain members and `std::function` over extra helper classes or abstraction layers, raw pointers owned by a manager over `std::shared_ptr`, plain `std::` exceptions. 
- Member fields should use `m_` prefix. In example width should be `m_width`. Use camel case otherwise ie `m_backBufferWidth`.

## C

- Types are PascalCase with the `Ris` prefix, for example `RisBuffer` and `RisResult`.
- Functions are `ris_` plus the C++ method name, so `Framework::addOnLoadContentListener` becomes `ris_addOnLoadContentListener`.
- Getters are done with `ris_get` plus the C++ method name, so `Framework::getWidth` becomes `ris_getWidth`.
- Setters are done with `ris_set` plus the C++ method name, so `Framework::setWidth` becomes `ris_setWidth`.
- Enum values are `RIS_` in upper snake case, for example `RIS_TEXTURE_FILTERING_NEAREST`.
- The C API is pure C. It wraps the C++ API one to one, uses opaque handles, and has no extra fields or functions.
