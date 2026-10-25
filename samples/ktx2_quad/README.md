# ktx2_quad

Loads `assets/test_basis_uastc.ktx2` with `ris::Texture::from_ktx2_file` and shows it as a textured quad with
`ris::SpriteBatch`, in front of a checkerboard so that the alpha channel is visible. Uses the KTX2 support of
[ris-ktx2-cpp](https://github.com/luka712/ris-ktx2-cpp).

The file is a 248x248 Basis Universal UASTC texture. The framework transcodes it to the best format the GPU supports:
ASTC 4x4 when `GL_KHR_texture_compression_astc_ldr` is available, otherwise ETC2 (BC7 is not used). The console shows
which one was picked. `--scale N` (default 2) displays the texture N times larger.

`assets/test_basis_uastc.ktx2` is the test file of ris-ktx2-cpp. It was made from `test.png`, "Texture mapping test
image" on Openclipart (<https://openclipart.org/detail/244429/texture-mapping-test-image>), released under CC0.
