//
// Created by luka on 26.10.26.
//

#include "data.hpp"

risColor RIS_CALL ris_lightPinkColor(void) {
    const ris::Color color = ris::Color::lightPink();
    return {color.r, color.g, color.b, color.a};
}
