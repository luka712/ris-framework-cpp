/* The sprite_batch sample written against the C API only (ris/ris.h), the way a C# or other language binding
 * uses the library: a framework with a load content and a render listener, a texture and the sprite batch.
 * Same result as the C++ sample "sprite_batch" (it also reads that sample's asset). Close the window to exit.
 */
#include <stdint.h>
#include <stdio.h>

#include <ris/ris.h>

#ifndef RIS_SAMPLES_DIR
#define RIS_SAMPLES_DIR "."
#endif

/* The state of the sample, given to the listeners as userData. */
typedef struct sample {
    ris_texture* parrots;   /* owned by the sample */
    float width, height;
} sample;

static void RIS_CALL loadContent(RisFramework* framework, void* userData) {
    sample* s = (sample*)userData;
    /* Kodak test image kodim23 (parrots), 512x341. See samples/sprite_batch/README.md for source and license. */
    s->parrots = ris_texture_load_image_file(RIS_SAMPLES_DIR "/sprite_batch/assets/kodim23.png", 1, 0, 0);
    if (!s->parrots) {
        fprintf(stderr, "error: loading kodim23.png failed: %s\n", ris_get_last_error());
        ris_quit(framework);
        return;
    }
    {
        ris_sampler_desc bilinear;
        ris_sampler_desc_default(&bilinear);
        bilinear.min_filter = RIS_FILTER_LINEAR;
        bilinear.mag_filter = RIS_FILTER_LINEAR;
        ris_texture_set_sampler(s->parrots, &bilinear);
    }
    s->width = (float)ris_texture_get_width(s->parrots);
    s->height = (float)ris_texture_get_height(s->parrots);
    printf("Loaded kodim23.png (%dx%d).\n", (int)s->width, (int)s->height);
}

static void RIS_CALL render(RisFramework* framework, void* userData) {
    sample* s = (sample*)userData;
    RisSpriteBatch* spriteBatch = ris_getSpriteBatch(framework);
    if (!s->parrots) return;
    ris_spriteBatchBegin(spriteBatch, NULL, RIS_BLEND_ALPHA);
    ris_spriteBatchDrawQuad(spriteBatch, s->parrots, 16, 16, s->width, s->height, 1, 1, 1, 1);
    ris_spriteBatchEnd(spriteBatch);
}

int main(void) {
    sample userData = {0};
    int exitCode = 0;
    RisFrameworkConfiguration config = ris_createFrameworkConfiguration();
    config.title = "ris sample: sprite_batch_c_api";
    config.backBufferWidth = 1280;
    config.backBufferHeight = 720;
    config.textureFiltering = RIS_TEXTURE_FILTERING_BILINEAR;

    RisFramework* framework = ris_createFramework(&config);
    if (!framework) {
        fprintf(stderr, "error: ris_createFramework failed: %s\n", ris_get_last_error());
        return 1;
    }

    ris_addOnLoadContentListener(framework, loadContent, &userData);
    ris_addOnRenderListener(framework, render, &userData);
    const RisColor pink = ris_lightPinkColor();
    ris_setClearColor(ris_getRenderer(framework), pink.r, pink.g, pink.b, pink.a);

    if (ris_initialize(framework) != RIS_OK) {
        fprintf(stderr, "error: ris_initialize failed: %s\n", ris_get_last_error());
        exitCode = 1;
    }
    if (!userData.parrots && exitCode == 0) exitCode = 1;

    ris_destroyTexture(userData.parrots);   /* before the framework: it needs the GL context */
    ris_destroyFramework(framework);
    return exitCode;
}
