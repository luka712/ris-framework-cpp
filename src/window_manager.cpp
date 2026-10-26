#include "ris/window_manager.hpp"

#include <SDL3/SDL.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglext_angle.h>
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>

#include <atomic>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>

#include "internal.hpp"

using namespace ris::detail;

namespace ris {
    
uint32_t WindowManager::s_updateIds = 0;
uint32_t WindowManager::s_renderIds = 0;
uint32_t WindowManager::s_resizeIds = 0;
uint32_t WindowManager::s_eventIds = 0;

WindowManager::WindowManager(const WindowDescriptor& desc) : m_windowDesc(desc) {
}

WindowManager::~WindowManager() {
    release();
}

void WindowManager::release() noexcept {

#if defined(__APPLE__)
    if (metalView_) SDL_Metal_DestroyView(static_cast<SDL_MetalView>(metalView_));
#endif
    if (m_window) SDL_DestroyWindow(m_window);
    m_window = nullptr;
    if (m_sdlInitialized) SDL_Quit();
    m_sdlInitialized = false;

}

void WindowManager::initializeForGles() {

    // Init sdl
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
    }
    m_sdlInitialized = true;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1); // Use double buffering.

    m_windowFlags |= SDL_WINDOW_OPENGL;

    m_window = SDL_CreateWindow(m_windowDesc.title.c_str(), m_windowDesc.width, m_windowDesc.height, m_windowFlags);
   if (!m_window) {
       throw std::runtime_error(std::string("SDL_CreateWindow failed: ") + SDL_GetError());
   }
}

void WindowManager::requireWindow() const {
    if (!m_window) throw std::logic_error("WindowManager: the window does not exist, call initializeForGles first");
}

glm::vec2 WindowManager::getWindowBounds() const {
    requireWindow();
    int w = 0, h = 0;
    SDL_GetWindowSizeInPixels(m_window, &w, &h);
    return {static_cast<float>(w), static_cast<float>(h)};
}

void WindowManager::setTitle(const std::string& title) {
    m_windowDesc.title = title;
    if (m_window) SDL_SetWindowTitle(m_window, title.c_str());
}

void WindowManager::setFullscreen(bool enabled) {
    requireWindow();
    SDL_SetWindowFullscreen(m_window, enabled);
}

void WindowManager::setVsync(bool enabled) {
    requireWindow();
    SDL_GL_SetSwapInterval(enabled ? 1 : 0);
}

void WindowManager::swapBuffers() {
    requireWindow();
    SDL_GL_SwapWindow(m_window);
}

void WindowManager::runEventLoop() {
    requireWindow();
    for (;;) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            detail::dispatch(onEventListeners_, e);
            if (e.type == SDL_EVENT_QUIT || e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
                quit_ = true;
            } else if (e.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
                detail::dispatch(onResizeListeners_, *this,
                                 glm::vec2(static_cast<float>(e.window.data1), static_cast<float>(e.window.data2)));
            }
        }
        if (quit_) break;

        detail::dispatch(onUpdateListeners_);
        detail::dispatch(onRenderListeners_);
        swapBuffers();
    }
    quit_ = false;
}

} // namespace ris
