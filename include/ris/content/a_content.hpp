//
// Created by luka on 25.10.26.
//

#ifndef RIS_FRAMEWORK_A_CONTENT_HPP
#define RIS_FRAMEWORK_A_CONTENT_HPP

#include "ris/core.hpp"

namespace ris {

/// The content base class, like `IContent` of ris-framework-ts: every piece of content (a texture, ...) derives from
/// it. In TypeScript `IContent` only adds `IDisposable`; here this is the virtual destructor: there is no
/// `dispose()`, content releases its resources when it is destroyed (RAII). Content cannot be copied.
class RIS_API AContent {
public:
    virtual ~AContent() = default;

    AContent(const AContent&) = delete;
    AContent& operator=(const AContent&) = delete;

protected:
    AContent() = default;
    AContent(AContent&&) noexcept = default;
    AContent& operator=(AContent&&) noexcept = default;
};

} // namespace ris

#endif //RIS_FRAMEWORK_A_CONTENT_HPP
