# ktx2_compare

Shows the same 248x248 image as three KTX2 files side by side, each loaded with `ris::Texture::from_ktx2_file`:

| panel | file | GPU format |
|-------|------|------------|
| left | `assets/test_astc4x4.ktx2` | ASTC 4x4 RGBA, 1 byte per pixel |
| middle | `assets/test_etc2.ktx2` | ETC2 RGBA, 1 byte per pixel |
| right | `assets/test_rgba8.ktx2` | RGBA8, uncompressed reference, 4 bytes per pixel |

The framework has no text rendering, so the panels are labelled on the console (format, size, GPU memory) and in
the window title. BC7 is not used.

A variant that the GL context cannot use is skipped: the console explains why and the panel is drawn red. For example
without `GL_KHR_texture_compression_astc_ldr` (try `MESA_EXTENSION_OVERRIDE=-GL_KHR_texture_compression_astc_ldr`
on Mesa) the ASTC panel is red and the other two still render. `--scale N` (default 2) displays each texture N times larger.

## Assets

The three files were produced from `ktx2_quad/assets/test_basis_uastc.ktx2` (UASTC) with the `ktx` tool of
KTX-Software, all with Zstandard supercompression:

```sh
ktx transcode --target astc     --zstd 19 test_basis_uastc.ktx2 test_astc4x4.ktx2
ktx transcode --target etc-rgba --zstd 19 test_basis_uastc.ktx2 test_etc2.ktx2
ktx transcode --target rgba8    --zstd 19 test_basis_uastc.ktx2 test_rgba8.ktx2
```

The image is `test.png` from Openclipart (<https://openclipart.org/detail/244429/texture-mapping-test-image>, CC0).
UASTC to ASTC is close to lossless, ETC2 is visibly lossier (on a Linux/Mesa run, ASTC matched the RGBA8 reference to
57 dB PSNR, ETC2 to 29 dB).
