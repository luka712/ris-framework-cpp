//
// Created by luka on 25.10.26.
//

#ifndef RIS_FRAMEWORK_GLES_CONTEXT_UTILITIES_H
#define RIS_FRAMEWORK_GLES_CONTEXT_UTILITIES_H
#include <string>
#include <vector>
#include <GLES3/gl3.h>

#include "ris/core.hpp"

    // TODO: doc comment
    struct RIS_API GlesContextUtilities {

        // TODO: doc comment
        std::string glString(GLenum name);

        // TODO: doc comment
        std::vector<std::string> getExtensions();

        // TODO: doc comment
        bool isExtensionSupported(const std::string& extension);
    };

#endif //RIS_FRAMEWORK_GLES_CONTEXT_UTILITIES_H
