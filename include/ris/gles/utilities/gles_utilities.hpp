//
// Created by luka on 25.10.26.
//

#ifndef RIS_FRAMEWORK_GLES_UTILITIES_HPP
#define RIS_FRAMEWORK_GLES_UTILITIES_HPP
// The utility headers of this folder (all of them, as before), plus GlesUtilities, the entry point to the new class based ones.
#include "ris/gles/utilities/anisotropic_filter_utilities.hpp"
#include "ris/gles/utilities/blending_utilities.hpp"
#include "ris/gles/utilities/gles_buffer_utilities.hpp"
#include "ris/gles/utilities/culling_utilities.hpp"
#include "ris/gles/utilities/debug_label_utilities.hpp"
#include "ris/gles/utilities/depth_attachment_type.hpp"
#include "ris/gles/utilities/error_utilities.hpp"
#include "ris/gles/utilities/framebuffer_utilities.hpp"
#include "ris/gles/utilities/gles_converter.hpp"
#include "ris/gles/utilities/program_utilities.hpp"
#include "ris/gles/utilities/renderbuffer_utilities.hpp"
#include "ris/gles/utilities/sampler_utilities.hpp"
#include "ris/gles/utilities/shader_utilities.hpp"
#include "ris/gles/utilities/gles_texture_utilities.hpp"
#include "gles_context_utilities.hpp"

// TODO doc comment
class RIS_API GlesUtilities {

    public:
        // TODO doc comment
        static GlesContextUtilities& context() {
            return m_contextUtilities;
        }

        /// The buffer utilities, `WebGlBufferUtilities` of ris-framework-ts.
        static ris::gles::GlesBufferUtilities& buffer() {
            return m_bufferUtilities;
        }

        /// The texture utilities, `WebGlTextureUtilities` of ris-framework-ts.
        static ris::gles::GlesTextureUtilities& texture() {
            return m_textureUtilities;
        }

    private:
        static GlesContextUtilities m_contextUtilities;
        static ris::gles::GlesBufferUtilities m_bufferUtilities;
        static ris::gles::GlesTextureUtilities m_textureUtilities;
};

#endif //RIS_FRAMEWORK_GLES_UTILITIES_HPP
