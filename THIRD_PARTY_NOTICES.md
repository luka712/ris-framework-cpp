# Third-Party Notices

ris-framework itself is released under the MIT license, see `LICENSE`.

The following libraries are linked into the shared library. Their full license texts are shipped in the `third_party/`
folder of every release archive (`share/ris_framework/third_party/`).

| Library | License | Notes |
| --- | --- | --- |
| SDL3 | zlib | window, events, input |
| ANGLE | BSD-3-Clause | OpenGL ES over D3D11 / Metal, Windows and macOS builds only |
| Khronos KTX-Software (libktx) | Apache-2.0 | via ris-ktx2, includes Basis Universal, astc-encoder, dfdutils |
| Ericsson ETC decoder (inside libktx) | Ericsson Texture Compression Software License Agreement | see the notice in ris-ktx2 |
| ris-ktx2-cpp | MIT | |
| spdlog, {fmt} | MIT | via ris-ktx2 |
| zstd | BSD-3-Clause | via libktx |
| stb_image | MIT / public domain | via ris-ktx2 |
| Vulkan headers | Apache-2.0 / MIT | only `VkFormat` values |

See the ris-ktx2-cpp `THIRD_PARTY_NOTICES.md` for the full text of the licenses of the KTX components.
On Linux the system EGL / GLESv2 (Mesa or the vendor driver) is loaded at runtime and is not redistributed.
