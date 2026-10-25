#include "ris/gles/utilities/extension_utilities.hpp"

#include <cstring>

#include "internal.hpp"

using namespace ris::detail;

namespace ris::gles::extension_utilities {

bool has_extension(const char* name) {
    require_context();
    if (!name || !*name) return false;
    const char* all = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
    if (!all) return false;
    const size_t len = std::strlen(name);
    for (const char* p = all; (p = std::strstr(p, name)) != nullptr; p += len) {
        const bool start_ok = p == all || p[-1] == ' ';
        const bool end_ok = p[len] == '\0' || p[len] == ' ';
        if (start_ok && end_ok) return true;
    }
    return false;
}

} // namespace ris::gles::extension_utilities
