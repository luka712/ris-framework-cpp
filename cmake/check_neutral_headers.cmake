# Fails if a header of the API neutral layers includes a backend, OpenGL or SDL header:
#  - ris/rendering/ (graphics API neutral): no ris/gles/, GL / EGL / ANGLE headers, graphics.hpp
#  - ris/input/ (windowing API neutral): no ris/sdl3/, SDL headers, window.hpp, graphics.hpp, GL headers
# Usage: cmake -DRIS_INCLUDE_DIR=<path to include/> -P check_neutral_headers.cmake
set(_bad "")
set(_checked 0)

file(GLOB_RECURSE _rendering_headers "${RIS_INCLUDE_DIR}/ris/rendering/*.hpp")
foreach(_header IN LISTS _rendering_headers)
    file(STRINGS "${_header}" _includes REGEX "^[ \t]*#[ \t]*include")
    foreach(_line IN LISTS _includes)
        if(_line MATCHES "ris/gles/|GLES|EGL|GL/|opengl|angle|graphics\\.hpp")
            string(APPEND _bad "  ${_header}: ${_line}\n")
        endif()
    endforeach()
    math(EXPR _checked "${_checked} + 1")
endforeach()

file(GLOB_RECURSE _input_headers "${RIS_INCLUDE_DIR}/ris/input/*.hpp")
foreach(_header IN LISTS _input_headers)
    file(STRINGS "${_header}" _includes REGEX "^[ \t]*#[ \t]*include")
    foreach(_line IN LISTS _includes)
        if(_line MATCHES "ris/gles/|ris/sdl3/|SDL|sdl3|GLES|EGL|GL/|opengl|angle|graphics\\.hpp|window\\.hpp")
            string(APPEND _bad "  ${_header}: ${_line}\n")
        endif()
    endforeach()
    math(EXPR _checked "${_checked} + 1")
endforeach()

if(_bad)
    message(FATAL_ERROR "API neutral headers include a backend header:\n${_bad}")
endif()
message(STATUS "checked ${_checked} headers of ris/rendering/ and ris/input/")
