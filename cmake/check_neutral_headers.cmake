# Fails if a header of the API neutral layers includes a backend, OpenGL or SDL header:
#  - ris/rendering/, ris/content/ and ris/data/ (graphics API neutral): no ris/gles/, GL / EGL / ANGLE headers, graphics.hpp
#  - ris/input/ and ris/time/ (SDL3 is integral to the engine, so there is no input / time backend split, but SDL
#    must stay private): no ris/gles/, SDL headers, window.hpp, graphics.hpp, GL headers
# Usage: cmake -DRIS_INCLUDE_DIR=<path to include/> -P check_neutral_headers.cmake
set(_bad "")
set(_checked 0)

file(GLOB_RECURSE _rendering_headers "${RIS_INCLUDE_DIR}/ris/rendering/*.hpp" "${RIS_INCLUDE_DIR}/ris/content/*.hpp"
     "${RIS_INCLUDE_DIR}/ris/data/*.hpp")
foreach(_header IN LISTS _rendering_headers)
    file(STRINGS "${_header}" _includes REGEX "^[ \t]*#[ \t]*include")
    foreach(_line IN LISTS _includes)
        if(_line MATCHES "ris/gles/|GLES|EGL|GL/|opengl|angle|graphics\\.hpp")
            string(APPEND _bad "  ${_header}: ${_line}\n")
        endif()
    endforeach()
    math(EXPR _checked "${_checked} + 1")
endforeach()

file(GLOB_RECURSE _input_headers "${RIS_INCLUDE_DIR}/ris/input/*.hpp" "${RIS_INCLUDE_DIR}/ris/time/*.hpp")
foreach(_header IN LISTS _input_headers)
    file(STRINGS "${_header}" _includes REGEX "^[ \t]*#[ \t]*include")
    foreach(_line IN LISTS _includes)
        if(_line MATCHES "ris/gles/|SDL|sdl3|GLES|EGL|GL/|opengl|angle|graphics\\.hpp|window\\.hpp")
            string(APPEND _bad "  ${_header}: ${_line}\n")
        endif()
    endforeach()
    math(EXPR _checked "${_checked} + 1")
endforeach()

if(_bad)
    message(FATAL_ERROR "API neutral headers include a backend header:\n${_bad}")
endif()
message(STATUS "checked ${_checked} headers of ris/rendering/, ris/content/, ris/data/, ris/input/ and ris/time/")
