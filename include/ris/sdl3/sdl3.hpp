#pragma once

// The SDL3 backend layer (windowing / input / time), next to `gles` (rendering). Everything here implements the neutral
// interfaces of ris/input/ and ris/time/; the SDL headers stay private to the library.

#include "ris/sdl3/input/input.hpp"
#include "ris/sdl3/time/time.hpp"
