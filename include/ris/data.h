/*
 * Plain data types shared by the C++ `ris::Framework` API and the C API (ris.h), the C counterpart of data.hpp.
 * Plain C, so it can be included from C and C++.
 */
#ifndef RIS_DATA_H
#define RIS_DATA_H

#include "ris_export.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Linear RGBA colour, components in [0, 1]. */
typedef struct risColor {
    float r;
    float g;
    float b;
    float a;
} RisColor;

/** The light pink of `ris::Color::lightPink()`. */
RIS_API RisColor RIS_CALL ris_lightPinkColor(void);

#ifdef __cplusplus
}
#endif

#endif /* RIS_DATA_H */
