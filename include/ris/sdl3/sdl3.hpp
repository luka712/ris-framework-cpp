#pragma once

// The SDL3 backend layer (windowing / input), next to `gles` (rendering). Everything here implements the neutral
// interfaces of ris/input/; the SDL headers stay private to the library.

#include "ris/sdl3/input/input.hpp"
