# sprite_batch_c_api

The `sprite_batch` sample written against the C API (`ris/ris.h`) only: `ris_createFrameworkConfiguration`,
`ris_createFramework`, a load content and a render listener (`ris_addOnLoadContentListener`, `ris_addOnRenderListener`,
with the state of the sample as `userData`), `ris_texture_load_image_file`, `ris_spriteBatch` with the
`ris_sprite_batch_*` functions, and `ris_initialize` for the loop. This is the code a C# or other language binding runs.
It renders the same image as `sprite_batch` and reads that sample's asset (`../sprite_batch/assets/kodim23.png`, see its
README for the source and license).
