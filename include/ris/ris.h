/*
 * ris-framework C API.
 *
 * A flat, blittable C ABI over the C++ framework, designed for P/Invoke (C#) and any other
 * language with C interop.
 *
 * Conventions
 *  - Everything is prefixed `ris_` / `RIS_`. Handles are opaque pointers.
 *  - Booleans are `uint32_t` (0 / 1), never C++ `bool`.
 *  - Enums are passed as `uint32_t` / `int32_t`, structs are plain, with no hidden padding
 *    and no pointers to C++ types.
 *  - Functions that create an object return NULL on failure; functions that can fail without
 *    a return object return a `ris_result` (RIS_OK == 0). Details: `ris_get_last_error()`
 *    (thread local).
 *  - Strings are UTF-8, NUL terminated. Returned `const char*` are owned by the framework and
 *    stay valid until the next call that returns a string on the same thread, unless noted.
 *  - Matrices are 16 floats, column-major (OpenGL layout).
 *  - Threading: all functions must be called from the thread that created the window
 *    (the "main" thread), except the logging, version and math functions.
 */
#ifndef RIS_H
#define RIS_H

#include <stddef.h>
#include <stdint.h>

#include "framework_configuration.h"
#include "ris_export.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Descriptor structs (ris_window_desc, ris_mesh_desc, ris_sampler_desc) start with `struct_size`,
 * which must be set to sizeof(the struct), ideally by calling the matching *_default function.
 * A `struct_size` smaller than the library's struct is rejected: the function returns NULL (create
 * functions) or RIS_ERROR_INVALID_ARGUMENT and sets ris_get_last_error(). A larger one is accepted
 * (a caller built against a newer header); the extra bytes are ignored. A NULL descriptor is an
 * invalid argument (RIS_ERROR_INVALID_ARGUMENT / NULL) with a specific last error message.
 * ris_sprite, ris_event and ris_vertex_attrib are fixed layout and have no `struct_size`.
 */

#define RIS_VERSION_MAJOR 0
#define RIS_VERSION_MINOR 1
#define RIS_VERSION_PATCH 0
#define RIS_VERSION_STRING "0.1.0"

/* ======================================================================================== */
/* Core: version, errors, logging, time                                                      */
/* ======================================================================================== */

typedef enum ris_result {
    RIS_OK = 0,
    RIS_ERROR_INVALID_ARGUMENT = -1,
    RIS_ERROR_NOT_INITIALIZED = -2,
    RIS_ERROR_PLATFORM = -3,       /* SDL / OS failure                                    */
    RIS_ERROR_GRAPHICS = -4,       /* EGL / ANGLE / OpenGL ES failure                      */
    RIS_ERROR_IO = -5,             /* file could not be read                              */
    RIS_ERROR_UNSUPPORTED = -6,
    RIS_ERROR_OUT_OF_MEMORY = -7,
    RIS_ERROR_COMPILE = -8,        /* shader compile / link failure (see last error)      */
    RIS_ERROR_KTX = -9             /* KTX2 loading / transcoding failure                  */
} ris_result;

RIS_API uint32_t RIS_CALL ris_get_version_major(void);
RIS_API uint32_t RIS_CALL ris_get_version_minor(void);
RIS_API uint32_t RIS_CALL ris_get_version_patch(void);
/** "major.minor.patch", static storage. */
RIS_API const char* RIS_CALL ris_get_version_string(void);

/**
 * Message of the last failed call on this thread; empty string if none.
 *
 * Lifetime of returned strings: the pointer returned by ris_get_last_error and by
 * ris_window_get_gl_info points into thread local storage that is overwritten by the next call
 * of either function on the same thread. Copy the string if you need it longer. Strings returned
 * by ris_get_version_string are static.
 */
RIS_API const char* RIS_CALL ris_get_last_error(void);
RIS_API void RIS_CALL ris_clear_last_error(void);

typedef enum ris_log_level {
    RIS_LOG_TRACE = 0,
    RIS_LOG_DEBUG = 1,
    RIS_LOG_INFO = 2,
    RIS_LOG_WARN = 3,
    RIS_LOG_ERROR = 4,
    RIS_LOG_OFF = 5
} ris_log_level;

/** The message pointer is only valid during the callback. May be called from any thread. */
typedef void(RIS_CALL* ris_log_fn)(uint32_t level, const char* message, void* user_data);

/** Pass NULL to restore the default (stderr). */
RIS_API void RIS_CALL ris_set_log_callback(ris_log_fn callback, void* user_data);
RIS_API void RIS_CALL ris_set_log_level(ris_log_level level);

/** Monotonic seconds since the first call to any time function. */
RIS_API double RIS_CALL ris_get_time_seconds(void);
RIS_API void RIS_CALL ris_sleep_ms(uint32_t milliseconds);

/* ======================================================================================== */
/* Window + OpenGL ES context (SDL3 + ANGLE)                                                 */
/* ======================================================================================== */

typedef struct ris_window ris_window;

typedef enum ris_window_flags {
    RIS_WINDOW_RESIZABLE = 1u << 0,
    RIS_WINDOW_FULLSCREEN = 1u << 1,
    RIS_WINDOW_BORDERLESS = 1u << 2,
    RIS_WINDOW_HIGH_DPI = 1u << 3,
    RIS_WINDOW_HIDDEN = 1u << 4,
    RIS_WINDOW_MAXIMIZED = 1u << 5
} ris_window_flags;

/** Which native API ANGLE translates OpenGL ES calls to. */
typedef enum ris_angle_backend {
    RIS_ANGLE_BACKEND_AUTO = 0,     /* Windows: D3D11, macOS/iOS: Metal, Linux: OpenGL     */
    RIS_ANGLE_BACKEND_OPENGL = 1,
    RIS_ANGLE_BACKEND_OPENGLES = 2,
    RIS_ANGLE_BACKEND_VULKAN = 3,
    RIS_ANGLE_BACKEND_D3D11 = 4,
    RIS_ANGLE_BACKEND_METAL = 5,
    RIS_ANGLE_BACKEND_NULL = 6      /* no rendering, for headless tests                    */
} ris_angle_backend;

typedef struct ris_window_desc {
    uint32_t struct_size;   /* sizeof(ris_window_desc), set by ris_window_desc_default       */
    int32_t width;
    int32_t height;
    uint32_t flags;         /* ris_window_flags                                              */
    uint32_t backend;       /* ris_angle_backend. RIS_ANGLE_BACKEND env var overrides AUTO.  */
    uint32_t vsync;         /* 0 / 1                                                         */
    uint32_t msaa_samples;  /* 0 = off, otherwise 2, 4, 8                                    */
    uint32_t depth_bits;    /* 0, 16, 24                                                     */
    uint32_t stencil_bits;  /* 0 or 8                                                        */
    uint32_t debug;         /* 0 / 1: request a debug context and log GL debug messages      */
    const char* title;      /* UTF-8, NULL = "ris-framework". Copied by ris_window_create, may be freed afterwards. */
} ris_window_desc;

/** Fills 1280x720, resizable, vsync on, 24 bit depth, 8 bit stencil, backend AUTO. */
RIS_API void RIS_CALL ris_window_desc_default(ris_window_desc* desc);

/**
 * Initialises SDL, creates the window, an ANGLE EGL display and an OpenGL ES 3.0+ context and
 * makes it current on the calling thread. Returns NULL on failure.
 * Only one window is supported in 0.1.0.
 */
RIS_API ris_window* RIS_CALL ris_window_create(const ris_window_desc* desc);
RIS_API void RIS_CALL ris_window_destroy(ris_window* window);

RIS_API void RIS_CALL ris_window_swap_buffers(ris_window* window);
RIS_API void RIS_CALL ris_window_set_vsync(ris_window* window, uint32_t enabled);
RIS_API void RIS_CALL ris_window_set_title(ris_window* window, const char* title);
RIS_API void RIS_CALL ris_window_set_fullscreen(ris_window* window, uint32_t enabled);
RIS_API void RIS_CALL ris_window_show(ris_window* window);
/** Size in screen coordinates. Either pointer may be NULL. */
RIS_API void RIS_CALL ris_window_get_size(const ris_window* window, int32_t* width, int32_t* height);
/** Size of the drawable in pixels (differs from the window size on high DPI displays). */
RIS_API void RIS_CALL ris_window_get_drawable_size(const ris_window* window, int32_t* width, int32_t* height);
RIS_API float RIS_CALL ris_window_get_content_scale(const ris_window* window);
RIS_API void RIS_CALL ris_window_set_text_input(ris_window* window, uint32_t enabled);
/** Ask the application to quit: queues a RIS_EVENT_QUIT event. */
RIS_API void RIS_CALL ris_window_request_quit(ris_window* window);

typedef enum ris_gl_info {
    RIS_GL_INFO_VENDOR = 0,
    RIS_GL_INFO_RENDERER = 1,
    RIS_GL_INFO_VERSION = 2,
    RIS_GL_INFO_GLSL_VERSION = 3,
    RIS_GL_INFO_EXTENSIONS = 4,
    RIS_GL_INFO_ANGLE_BACKEND = 5   /* "OpenGL", "D3D11", "Metal", ...                     */
} ris_gl_info;

/** The string is valid until the next ris_window_get_gl_info / ris_get_last_error call on this thread. */
RIS_API const char* RIS_CALL ris_window_get_gl_info(const ris_window* window, ris_gl_info which);
/** 1 if the OpenGL ES extension (e.g. "GL_KHR_texture_compression_astc_ldr") is available. */
RIS_API uint32_t RIS_CALL ris_window_has_gl_extension(const ris_window* window, const char* name);

/* ======================================================================================== */
/* Events and input                                                                          */
/* ======================================================================================== */

typedef enum ris_event_type {
    RIS_EVENT_NONE = 0,
    RIS_EVENT_QUIT = 1,
    RIS_EVENT_WINDOW_RESIZED = 2,       /* width, height = new drawable size in pixels     */
    RIS_EVENT_WINDOW_FOCUS_GAINED = 3,
    RIS_EVENT_WINDOW_FOCUS_LOST = 4,
    RIS_EVENT_KEY_DOWN = 5,             /* scancode, keycode, mods, repeat                 */
    RIS_EVENT_KEY_UP = 6,
    RIS_EVENT_TEXT_INPUT = 7,           /* text                                            */
    RIS_EVENT_MOUSE_MOVE = 8,           /* x, y, dx, dy                                    */
    RIS_EVENT_MOUSE_BUTTON_DOWN = 9,    /* button, clicks, x, y                            */
    RIS_EVENT_MOUSE_BUTTON_UP = 10,
    RIS_EVENT_MOUSE_WHEEL = 11          /* dx, dy                                          */
} ris_event_type;

typedef enum ris_key_mod {
    RIS_MOD_SHIFT = 1u << 0,
    RIS_MOD_CTRL = 1u << 1,
    RIS_MOD_ALT = 1u << 2,
    RIS_MOD_GUI = 1u << 3
} ris_key_mod;

/** Mouse buttons: 1 = left, 2 = middle, 3 = right, 4 / 5 = extra. Bit (button - 1) in masks. */
typedef struct ris_event {
    uint32_t type;          /* ris_event_type                                              */
    uint32_t repeat;        /* key repeat flag                                             */
    uint64_t timestamp_ns;
    int32_t scancode;       /* ris_scancode (USB HID usage, same values as SDL_Scancode)   */
    int32_t keycode;        /* layout dependent key (SDL_Keycode)                          */
    uint32_t mods;          /* ris_key_mod bit mask                                        */
    int32_t button;
    float x;
    float y;
    float dx;
    float dy;
    int32_t width;
    int32_t height;
    int32_t clicks;
    int32_t reserved;
    char text[32];          /* UTF-8, RIS_EVENT_TEXT_INPUT only                            */
} ris_event;

/** Pumps the OS event queue. Returns 1 and fills `out_event` while events are pending. */
RIS_API uint32_t RIS_CALL ris_window_poll_event(ris_window* window, ris_event* out_event);

/** A subset of common scancodes; the full set is SDL_Scancode (SDL3 `SDL_scancode.h`). */
typedef enum ris_scancode {
    RIS_SCANCODE_A = 4, RIS_SCANCODE_B = 5, RIS_SCANCODE_C = 6, RIS_SCANCODE_D = 7,
    RIS_SCANCODE_E = 8, RIS_SCANCODE_F = 9, RIS_SCANCODE_G = 10, RIS_SCANCODE_H = 11,
    RIS_SCANCODE_I = 12, RIS_SCANCODE_J = 13, RIS_SCANCODE_K = 14, RIS_SCANCODE_L = 15,
    RIS_SCANCODE_M = 16, RIS_SCANCODE_N = 17, RIS_SCANCODE_O = 18, RIS_SCANCODE_P = 19,
    RIS_SCANCODE_Q = 20, RIS_SCANCODE_R = 21, RIS_SCANCODE_S = 22, RIS_SCANCODE_T = 23,
    RIS_SCANCODE_U = 24, RIS_SCANCODE_V = 25, RIS_SCANCODE_W = 26, RIS_SCANCODE_X = 27,
    RIS_SCANCODE_Y = 28, RIS_SCANCODE_Z = 29,
    RIS_SCANCODE_1 = 30, RIS_SCANCODE_2 = 31, RIS_SCANCODE_3 = 32, RIS_SCANCODE_4 = 33,
    RIS_SCANCODE_5 = 34, RIS_SCANCODE_6 = 35, RIS_SCANCODE_7 = 36, RIS_SCANCODE_8 = 37,
    RIS_SCANCODE_9 = 38, RIS_SCANCODE_0 = 39,
    RIS_SCANCODE_RETURN = 40, RIS_SCANCODE_ESCAPE = 41, RIS_SCANCODE_BACKSPACE = 42,
    RIS_SCANCODE_TAB = 43, RIS_SCANCODE_SPACE = 44,
    RIS_SCANCODE_RIGHT = 79, RIS_SCANCODE_LEFT = 80, RIS_SCANCODE_DOWN = 81, RIS_SCANCODE_UP = 82,
    RIS_SCANCODE_LCTRL = 224, RIS_SCANCODE_LSHIFT = 225, RIS_SCANCODE_LALT = 226
} ris_scancode;

/** Polled keyboard / mouse state, valid after events were pumped with ris_window_poll_event. */
RIS_API uint32_t RIS_CALL ris_input_key_down(int32_t scancode);
/** Returns the button bit mask (bit 0 = left). Either pointer may be NULL. */
RIS_API uint32_t RIS_CALL ris_input_mouse_state(float* x, float* y);

/* ======================================================================================== */
/* Graphics state                                                                            */
/* ======================================================================================== */

typedef enum ris_clear_flags {
    RIS_CLEAR_COLOR = 1u << 0,
    RIS_CLEAR_DEPTH = 1u << 1,
    RIS_CLEAR_STENCIL = 1u << 2
} ris_clear_flags;

typedef enum ris_blend_mode {
    RIS_BLEND_NONE = 0,
    RIS_BLEND_ALPHA = 1,            /* src*a + dst*(1-a)                                   */
    RIS_BLEND_PREMULTIPLIED = 2,    /* src + dst*(1-a)                                     */
    RIS_BLEND_ADDITIVE = 3,
    RIS_BLEND_MULTIPLY = 4
} ris_blend_mode;

typedef enum ris_compare_func {
    RIS_COMPARE_NEVER = 0,
    RIS_COMPARE_LESS = 1,
    RIS_COMPARE_EQUAL = 2,
    RIS_COMPARE_LESS_EQUAL = 3,
    RIS_COMPARE_GREATER = 4,
    RIS_COMPARE_NOT_EQUAL = 5,
    RIS_COMPARE_GREATER_EQUAL = 6,
    RIS_COMPARE_ALWAYS = 7
} ris_compare_func;

typedef enum ris_cull_mode {
    RIS_CULL_NONE = 0,
    RIS_CULL_BACK = 1,
    RIS_CULL_FRONT = 2
} ris_cull_mode;

RIS_API void RIS_CALL ris_gfx_viewport(int32_t x, int32_t y, int32_t width, int32_t height);
RIS_API void RIS_CALL ris_gfx_set_scissor(uint32_t enabled, int32_t x, int32_t y, int32_t width, int32_t height);
RIS_API void RIS_CALL ris_gfx_clear(float r, float g, float b, float a, uint32_t clear_flags);
RIS_API void RIS_CALL ris_gfx_set_blend_mode(uint32_t blend_mode);
RIS_API void RIS_CALL ris_gfx_set_depth_state(uint32_t test_enabled, uint32_t write_enabled, uint32_t compare_func);
/** Counter clockwise triangles are front facing. */
RIS_API void RIS_CALL ris_gfx_set_cull_mode(uint32_t cull_mode);
/**
 * Reads RGBA8 pixels from the current framebuffer (origin bottom left). The buffer needs
 * width * height * 4 bytes; `out_size` is its size in bytes. Returns RIS_ERROR_INVALID_ARGUMENT
 * and writes nothing if it is too small.
 */
RIS_API ris_result RIS_CALL ris_gfx_read_pixels(int32_t x, int32_t y, int32_t width, int32_t height, void* out_rgba8, size_t out_size);

/* ---- buffers ----------------------------------------------------------------------------- */

typedef struct ris_buffer ris_buffer;

typedef enum ris_buffer_type {
    RIS_BUFFER_VERTEX = 0,
    RIS_BUFFER_INDEX = 1,
    RIS_BUFFER_UNIFORM = 2
} ris_buffer_type;

typedef enum ris_buffer_usage {
    RIS_USAGE_STATIC = 0,   /* written once, drawn many times                              */
    RIS_USAGE_DYNAMIC = 1,  /* updated occasionally                                        */
    RIS_USAGE_STREAM = 2    /* updated every frame                                         */
} ris_buffer_usage;

/** `data` may be NULL to allocate uninitialised storage of `size` bytes. */
RIS_API ris_buffer* RIS_CALL ris_buffer_create(uint32_t type, uint32_t usage, const void* data, size_t size);
RIS_API ris_result RIS_CALL ris_buffer_update(ris_buffer* buffer, size_t offset, const void* data, size_t size);
RIS_API size_t RIS_CALL ris_buffer_get_size(const ris_buffer* buffer);
/** Binds a uniform buffer range to a uniform block binding point. */
RIS_API void RIS_CALL ris_buffer_bind_base(const ris_buffer* buffer, uint32_t binding);
RIS_API void RIS_CALL ris_buffer_destroy(ris_buffer* buffer);

/* ---- shaders ----------------------------------------------------------------------------- */

typedef struct ris_shader ris_shader;

/**
 * Compiles and links a program from GLSL ES 3.00 (`#version 300 es`) sources.
 * On failure returns NULL and the info log is in ris_get_last_error().
 */
RIS_API ris_shader* RIS_CALL ris_shader_create(const char* vertex_source, const char* fragment_source);
RIS_API void RIS_CALL ris_shader_use(ris_shader* shader);
/** -1 if the uniform does not exist (setting it is then a no-op). */
RIS_API int32_t RIS_CALL ris_shader_get_uniform_location(const ris_shader* shader, const char* name);
RIS_API int32_t RIS_CALL ris_shader_get_attrib_location(const ris_shader* shader, const char* name);
RIS_API void RIS_CALL ris_shader_set_int(ris_shader* shader, int32_t location, int32_t v);
RIS_API void RIS_CALL ris_shader_set_float(ris_shader* shader, int32_t location, float v);
RIS_API void RIS_CALL ris_shader_set_vec2(ris_shader* shader, int32_t location, float x, float y);
RIS_API void RIS_CALL ris_shader_set_vec3(ris_shader* shader, int32_t location, float x, float y, float z);
RIS_API void RIS_CALL ris_shader_set_vec4(ris_shader* shader, int32_t location, float x, float y, float z, float w);
RIS_API void RIS_CALL ris_shader_set_mat4(ris_shader* shader, int32_t location, const float* mat4_column_major);
/** Assigns a named uniform block to a binding point, see ris_buffer_bind_base. */
RIS_API ris_result RIS_CALL ris_shader_bind_uniform_block(ris_shader* shader, const char* block_name, uint32_t binding);
RIS_API void RIS_CALL ris_shader_destroy(ris_shader* shader);

/* ---- meshes (vertex array objects) ------------------------------------------------------- */

typedef struct ris_mesh ris_mesh;

#define RIS_MAX_VERTEX_BUFFERS 4
#define RIS_MAX_VERTEX_ATTRIBS 16

typedef enum ris_attrib_type {
    RIS_ATTRIB_FLOAT = 0,
    RIS_ATTRIB_UBYTE = 1,
    RIS_ATTRIB_BYTE = 2,
    RIS_ATTRIB_USHORT = 3,
    RIS_ATTRIB_SHORT = 4,
    RIS_ATTRIB_UINT = 5,
    RIS_ATTRIB_INT = 6
} ris_attrib_type;

typedef enum ris_index_type {
    RIS_INDEX_NONE = 0,
    RIS_INDEX_UINT16 = 1,
    RIS_INDEX_UINT32 = 2
} ris_index_type;

typedef enum ris_topology {
    RIS_TOPOLOGY_TRIANGLES = 0,
    RIS_TOPOLOGY_TRIANGLE_STRIP = 1,
    RIS_TOPOLOGY_TRIANGLE_FAN = 2,
    RIS_TOPOLOGY_LINES = 3,
    RIS_TOPOLOGY_LINE_STRIP = 4,
    RIS_TOPOLOGY_POINTS = 5
} ris_topology;

typedef struct ris_vertex_attrib {
    uint32_t location;      /* layout(location = N) in the vertex shader                   */
    uint32_t components;    /* 1..4                                                        */
    uint32_t type;          /* ris_attrib_type                                             */
    uint32_t normalized;    /* 0 / 1, integer types only (converted to float)              */
    uint32_t offset;        /* byte offset inside the vertex                               */
    uint32_t buffer_slot;   /* index into vertex_buffers                                   */
    uint32_t divisor;       /* 0 = per vertex, N = per N instances                         */
    uint32_t as_integer;    /* 0 / 1: feed an `ivec` / `uvec` input (integer types only)   */
} ris_vertex_attrib;

typedef struct ris_mesh_desc {
    uint32_t struct_size;       /* sizeof(ris_mesh_desc), set by ris_mesh_desc_default     */
    uint32_t _reserved0;        /* explicit padding, set to 0                              */
    const ris_vertex_attrib* attribs;
    uint32_t attrib_count;
    uint32_t vertex_buffer_count;
    ris_buffer* vertex_buffers[RIS_MAX_VERTEX_BUFFERS];
    uint32_t vertex_strides[RIS_MAX_VERTEX_BUFFERS];
    ris_buffer* index_buffer;   /* may be NULL                                             */
    uint32_t index_type;        /* ris_index_type                                          */
    uint32_t _reserved1;        /* explicit tail padding, set to 0                         */
} ris_mesh_desc;

/** Zeroes `desc` and sets struct_size. */
RIS_API void RIS_CALL ris_mesh_desc_default(ris_mesh_desc* desc);

RIS_API ris_mesh* RIS_CALL ris_mesh_create(const ris_mesh_desc* desc);
/** `first` is the first vertex, or the first index when the mesh is indexed. instance_count 0 => 1. */
RIS_API void RIS_CALL ris_mesh_draw(const ris_mesh* mesh, uint32_t topology, uint32_t first, uint32_t count, uint32_t instance_count);
RIS_API void RIS_CALL ris_mesh_destroy(ris_mesh* mesh);

/* ---- textures ---------------------------------------------------------------------------- */

typedef struct ris_texture ris_texture;

typedef enum ris_filter {
    RIS_FILTER_NEAREST = 0,
    RIS_FILTER_LINEAR = 1,
    RIS_FILTER_NEAREST_MIPMAP_NEAREST = 2,
    RIS_FILTER_LINEAR_MIPMAP_NEAREST = 3,
    RIS_FILTER_NEAREST_MIPMAP_LINEAR = 4,
    RIS_FILTER_LINEAR_MIPMAP_LINEAR = 5
} ris_filter;

typedef enum ris_wrap {
    RIS_WRAP_REPEAT = 0,
    RIS_WRAP_CLAMP = 1,
    RIS_WRAP_MIRROR = 2
} ris_wrap;

typedef struct ris_sampler_desc {
    uint32_t struct_size;   /* sizeof(ris_sampler_desc), set by ris_sampler_desc_default  */
    uint32_t min_filter;    /* ris_filter, mipmap filters need mip levels                  */
    uint32_t mag_filter;    /* RIS_FILTER_NEAREST or RIS_FILTER_LINEAR                     */
    uint32_t wrap_u;
    uint32_t wrap_v;
} ris_sampler_desc;

/** Linear mipmap min filter, linear mag filter, clamp: the same defaults as ris::SamplerDesc. */
RIS_API void RIS_CALL ris_sampler_desc_default(ris_sampler_desc* desc);

/** `pixels`: width*height RGBA8, may be NULL (uninitialised). `srgb`: treat data as sRGB. */
RIS_API ris_texture* RIS_CALL ris_texture_create_rgba8(int32_t width, int32_t height, const void* pixels, uint32_t generate_mipmaps, uint32_t srgb);
/** Loads PNG / JPG / BMP / TGA ... (stb_image through ris-ktx2) as an RGBA8 texture. */
RIS_API ris_texture* RIS_CALL ris_texture_load_image_file(const char* path, uint32_t generate_mipmaps, uint32_t srgb, uint32_t flip_vertically);
/**
 * Loads a KTX2 file (ris-ktx2 / libktx). Basis Universal (ETC1S / UASTC) textures are
 * transcoded to the best format the GPU supports: ASTC, then ETC2, with RGBA8 as the fallback.
 * All stored mip levels are uploaded. Only 2D textures are supported in 0.1.0.
 */
RIS_API ris_texture* RIS_CALL ris_texture_load_ktx2_file(const char* path);
RIS_API ris_texture* RIS_CALL ris_texture_load_ktx2_memory(const void* data, size_t size);
/** Returns RIS_OK, or RIS_ERROR_INVALID_ARGUMENT (null argument, or `struct_size` smaller than sizeof(ris_sampler_desc)) with the last error set. */
RIS_API ris_result RIS_CALL ris_texture_set_sampler(ris_texture* texture, const ris_sampler_desc* sampler);
RIS_API void RIS_CALL ris_texture_bind(const ris_texture* texture, uint32_t unit);
RIS_API int32_t RIS_CALL ris_texture_get_width(const ris_texture* texture);
RIS_API int32_t RIS_CALL ris_texture_get_height(const ris_texture* texture);
RIS_API uint32_t RIS_CALL ris_texture_get_mip_levels(const ris_texture* texture);
/** The OpenGL texture name, for interop with native code. */
RIS_API uint32_t RIS_CALL ris_texture_get_gl_handle(const ris_texture* texture);
RIS_API void RIS_CALL ris_destroyTexture(ris_texture* texture);

/* ---- render targets ---------------------------------------------------------------------- */

typedef struct ris_render_target ris_render_target;

RIS_API ris_render_target* RIS_CALL ris_render_target_create(int32_t width, int32_t height, uint32_t with_depth_stencil);
/**
 * Binds the target and sets the viewport to its size. NULL binds the window back buffer and
 * restores the viewport that was active before the first target was bound.
 */
RIS_API void RIS_CALL ris_render_target_bind(const ris_render_target* target);
/** The colour attachment. Owned by the render target: do not destroy it. */
RIS_API ris_texture* RIS_CALL ris_render_target_get_texture(const ris_render_target* target);
RIS_API void RIS_CALL ris_render_target_destroy(ris_render_target* target);

/* ======================================================================================== */
/* 2D sprite batch                                                                           */
/* ======================================================================================== */

typedef struct ris_sprite_batch ris_sprite_batch;

typedef struct ris_sprite {
    ris_texture* texture;   /* NULL = 1x1 white                                            */
    float x, y;             /* position of the origin point                                */
    float width, height;
    float u0, v0, u1, v1;   /* source rectangle in texture coordinates (0,0 = top left)    */
    float r, g, b, a;       /* tint                                                        */
    float rotation;         /* radians, counter clockwise on screen                        */
    float origin_x, origin_y; /* pivot, in pixels relative to the sprite's top left corner */
    uint32_t _reserved;     /* explicit tail padding (the struct is 72 bytes), set to 0   */
} ris_sprite;

RIS_API ris_sprite_batch* RIS_CALL ris_sprite_batch_create(uint32_t max_sprites_per_flush);
/**
 * Starts a batch. `view_projection` is a column-major mat4, or NULL for a pixel space
 * projection of the current viewport with (0,0) at the top left and y pointing down.
 */
RIS_API void RIS_CALL ris_spriteBatchBegin(ris_sprite_batch* batch, const float* view_projection, uint32_t blend_mode);
RIS_API void RIS_CALL ris_sprite_batch_draw(ris_sprite_batch* batch, const ris_sprite* sprite);
/** Textured, tinted, axis aligned quad covering the whole texture. */
RIS_API void RIS_CALL ris_spriteBatchDrawQuad(ris_sprite_batch* batch, ris_texture* texture, float x, float y, float width, float height, float r, float g, float b, float a);
RIS_API void RIS_CALL ris_spriteBatchEnd(ris_sprite_batch* batch);
RIS_API void RIS_CALL ris_sprite_batch_destroy(ris_sprite_batch* batch);

/* ======================================================================================== */
/* Framework: window, game loop and listeners (wraps ris::Framework)                         */
/* ======================================================================================== */

/*
 * A C wrapper over the C++ `ris::Framework`. It grows with what the C samples need.
 *
 * Naming convention: a function that wraps a C++ method is `ris_` + the name of that method
 * (`Framework::addOnRenderListener` -> `ris_addOnRenderListener`, `Framework::initialize` ->
 * `ris_initialize`, `Renderer::setClearColor` -> `ris_setClearColor`), and its first argument is the
 * object. Creating and destroying have their own names: `ris_createFramework`,
 * `ris_createFrameworkConfiguration`, `ris_destroyFramework`.
 *
 * While a callback runs the OpenGL ES context is current, so the `ris_texture_*`, `ris_sprite_batch_*` ...
 * functions can be used inside it. Use everything from the thread that called `ris_createFramework`.
 * Callbacks use RIS_CALL (cdecl): in C# mark the delegate with
 * `[UnmanagedFunctionPointer(CallingConvention.Cdecl)]` and keep it alive until the framework is destroyed.
 * Exceptions must not leave a callback.
 */

typedef struct ris_framework ris_framework;
/** The frame level state of the framework (clear colour), owned by it, see ris_renderer. */
typedef struct ris_Renderer ris_Renderer;

/** Listener of the load content and render events. `userData` is the pointer given to the add function. */
typedef void(RIS_CALL* ris_frameworkCallback)(ris_framework* framework, void* userData);

/** The defaults: "Ris Framework", 800x600, trilinear. The only place that sets them, also for the C++ `ris::Framework`. */
RIS_API FrameworkConfiguration RIS_CALL ris_createFrameworkConfiguration(void);
/** Opens the window and creates the graphics device and the sprite batch. Returns NULL on failure (see ris_get_last_error). Free it with ris_destroyFramework. */
RIS_API ris_framework* RIS_CALL ris_createFramework(const FrameworkConfiguration* config);
/** Closes the window and frees the framework. Destroy what you created while it ran (textures, ...) first. NULL is ignored. */
RIS_API void RIS_CALL ris_destroyFramework(ris_framework* framework);

/** Runs once at the start of ris_initialize, to load content. Returns the id of the listener (1, 2, ...), 0 for a NULL argument. */
RIS_API uint32_t RIS_CALL ris_addOnLoadContentListener(ris_framework* framework, ris_frameworkCallback callback, void* userData);
/** Runs every frame after the screen is cleared, to draw. Returns the id of the listener (1, 2, ...), 0 for a NULL argument. */
RIS_API uint32_t RIS_CALL ris_addOnRenderListener(ris_framework* framework, ris_frameworkCallback callback, void* userData);

/** Runs the load content listeners, then the loop until the window is closed or ris_quit is called. */
RIS_API ris_result RIS_CALL ris_initialize(ris_framework* framework);
/** Asks the loop to stop after the current frame. */
RIS_API void RIS_CALL ris_quit(ris_framework* framework);

/** The sprite batch of the framework, owned by it: do not destroy it. */
RIS_API ris_sprite_batch* RIS_CALL ris_getSpriteBatch(ris_framework* framework);
/** The renderer of the framework, owned by it. */
RIS_API ris_Renderer* RIS_CALL ris_getRenderer(ris_framework* framework);
/** The colour the back buffer is cleared with every frame; black by default. */
RIS_API void RIS_CALL ris_setClearColor(ris_Renderer* renderer, float r, float g, float b, float a);

/* ======================================================================================== */
/* Math helpers (column-major float[16])                                                     */
/* ======================================================================================== */

RIS_API void RIS_CALL ris_mat4_identity(float* out);
RIS_API void RIS_CALL ris_mat4_multiply(float* out, const float* a, const float* b);
RIS_API void RIS_CALL ris_mat4_translation(float* out, float x, float y, float z);
RIS_API void RIS_CALL ris_mat4_scale(float* out, float x, float y, float z);
RIS_API void RIS_CALL ris_mat4_rotation_x(float* out, float radians);
RIS_API void RIS_CALL ris_mat4_rotation_y(float* out, float radians);
RIS_API void RIS_CALL ris_mat4_rotation_z(float* out, float radians);
RIS_API void RIS_CALL ris_mat4_ortho(float* out, float left, float right, float bottom, float top, float near_z, float far_z);
RIS_API void RIS_CALL ris_mat4_perspective(float* out, float fov_y_radians, float aspect, float near_z, float far_z);
RIS_API void RIS_CALL ris_mat4_look_at(float* out, float eye_x, float eye_y, float eye_z, float target_x, float target_y, float target_z, float up_x, float up_y, float up_z);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RIS_H */
