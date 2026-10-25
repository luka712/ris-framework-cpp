#pragma once

// OpenGL ES utilities, mirroring the ris-framework-ts `webgl/utilities` folder (`WebGlUtilities` and the
// utility classes it aggregates). Each utility lives in its own header of this folder, this header includes
// all of them. In TypeScript they are static members of `WebGlUtilities` (`WebGlUtilities.buffer.…`), here
// they are namespaces under ris::gles (`ris::gles::buffer_utilities::…`).
//
// All of them need a ris::Window, see the rules in rendering/buffers/a_buffer.hpp. Except for the converter
// they throw Error(NotInitialized) without one.

#include "ris/gles/utilities/anisotropic_filter_utilities.hpp"
#include "ris/gles/utilities/blending_utilities.hpp"
#include "ris/gles/utilities/buffer_utilities.hpp"
#include "ris/gles/utilities/culling_utilities.hpp"
#include "ris/gles/utilities/debug_label_utilities.hpp"
#include "ris/gles/utilities/depth_attachment_type.hpp"
#include "ris/gles/utilities/error_utilities.hpp"
#include "ris/gles/utilities/extension_utilities.hpp"
#include "ris/gles/utilities/framebuffer_utilities.hpp"
#include "ris/gles/utilities/gles_converter.hpp"
#include "ris/gles/utilities/program_utilities.hpp"
#include "ris/gles/utilities/renderbuffer_utilities.hpp"
#include "ris/gles/utilities/sampler_utilities.hpp"
#include "ris/gles/utilities/shader_utilities.hpp"
#include "ris/gles/utilities/texture_utilities.hpp"
