cmake_minimum_required(VERSION 3.30)
project(ris_framework
        VERSION 0.1.0
        DESCRIPTION "C++ OpenGL ES framework (SDL3 + ANGLE) with a flat C API for P/Invoke"
        HOMEPAGE_URL "https://github.com/luka712/ris-framework-cpp"
        LANGUAGES C CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(FetchContent)

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
if(NOT DEFINED BUILD_SHARED_LIBS)
    # Shared by default: the C API is primarily consumed through P/Invoke.
    set(BUILD_SHARED_LIBS ON)
endif()

option(RIS_FRAMEWORK_BUILD_TESTS "Build the Catch2 test executable" ${PROJECT_IS_TOP_LEVEL})
option(RIS_FRAMEWORK_BUILD_SAMPLES "Build the samples" ${PROJECT_IS_TOP_LEVEL})
option(RIS_FRAMEWORK_INSTALL "Generate install rules" ON)
set(RIS_KTX2_SOURCE_DIR "" CACHE PATH "Use a local ris-ktx2-cpp checkout instead of fetching it")
set(RIS_KTX2_GIT_TAG "fd96cf4fa14f28ca08b59f86152c730a16c4e99e" CACHE STRING
    "ris-ktx2-cpp commit fetched when no local checkout or installed package is used (branch 0.1.0)")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ---------------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------------
find_package(SDL3 CONFIG REQUIRED)
find_package(unofficial-angle CONFIG REQUIRED)  # headers everywhere, libraries on Windows / macOS
if(UNIX AND NOT APPLE)
    # The vcpkg ANGLE port builds no rendering backend on Linux (no GL, Vulkan or Null display), so
    # link the system GLVND EGL + GLESv2 instead. ANGLE is auto-detected at runtime if it is the
    # libEGL that gets loaded.
    find_package(OpenGL REQUIRED COMPONENTS EGL)
    find_library(RIS_GLESV2_LIBRARY NAMES GLESv2 REQUIRED)
    set(_ris_gles_libs OpenGL::EGL ${RIS_GLESV2_LIBRARY})
else()
    set(_ris_gles_libs unofficial::angle::libEGL unofficial::angle::libGLESv2)
endif()

# ris-ktx2: local checkout, installed package, or fetched. When it is built here it is always
# linked statically so that libris_framework stays a single shared library.
if(RIS_KTX2_SOURCE_DIR)
    set(_ris_ktx2_build_shared ${BUILD_SHARED_LIBS})
    set(BUILD_SHARED_LIBS OFF)
    set(RIS_KTX2_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(RIS_KTX2_INSTALL OFF CACHE BOOL "" FORCE)
    add_subdirectory("${RIS_KTX2_SOURCE_DIR}" ris_ktx2 EXCLUDE_FROM_ALL)
    set(BUILD_SHARED_LIBS ${_ris_ktx2_build_shared})
else()
    find_package(ris_ktx2 CONFIG QUIET)
    if(NOT ris_ktx2_FOUND)
        message(STATUS "ris_ktx2 package not found, fetching ris-ktx2-cpp@${RIS_KTX2_GIT_TAG}")
        set(_ris_ktx2_build_shared ${BUILD_SHARED_LIBS})
        set(BUILD_SHARED_LIBS OFF)
        set(RIS_KTX2_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(RIS_KTX2_INSTALL OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(ris_ktx2
                GIT_REPOSITORY https://github.com/luka712/ris-ktx2-cpp.git
                GIT_TAG ${RIS_KTX2_GIT_TAG}
                EXCLUDE_FROM_ALL)
        FetchContent_MakeAvailable(ris_ktx2)
        set(BUILD_SHARED_LIBS ${_ris_ktx2_build_shared})
    endif()
endif()

# ---------------------------------------------------------------------------
# Library
# ---------------------------------------------------------------------------
set(TARGET_NAME ris_framework)

add_library(${TARGET_NAME}
        src/core.cpp
        src/math.cpp
        src/window.cpp
        src/graphics.cpp
        src/texture.cpp
        src/sprite_batch.cpp
        src/c_api.cpp
)
add_library(ris_framework::ris_framework ALIAS ${TARGET_NAME})

target_compile_features(${TARGET_NAME} PUBLIC cxx_std_20)
target_include_directories(${TARGET_NAME}
        PUBLIC
            "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
            "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
        PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

if(UNIX AND NOT APPLE)
    target_compile_definitions(${TARGET_NAME} PRIVATE KHRONOS_STATIC)
endif()

# Everything below is private: SDL3, ANGLE and ris-ktx2 are implementation details and are linked
# into the library, the public headers expose none of their types.
if(TARGET SDL3::SDL3-static)
    set(_ris_sdl3_target SDL3::SDL3-static)
else()
    set(_ris_sdl3_target SDL3::SDL3)
endif()

target_link_libraries(${TARGET_NAME} PRIVATE
        ${_ris_sdl3_target}
        ${_ris_gles_libs}
        ris_ktx2::ris_ktx2
)

if(BUILD_SHARED_LIBS)
    target_compile_definitions(${TARGET_NAME} PRIVATE RIS_BUILDING)
    if(UNIX AND NOT APPLE)
        # Do not re-export the statically linked SDL3 / ANGLE / libktx symbols.
        target_link_options(${TARGET_NAME} PRIVATE "LINKER:--exclude-libs,ALL")
    endif()
else()
    target_compile_definitions(${TARGET_NAME} PUBLIC RIS_STATIC)
endif()

if(WIN32)
    target_compile_definitions(${TARGET_NAME} PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()
if(MSVC)
    target_compile_options(${TARGET_NAME} PRIVATE /W4 /permissive-)
else()
    target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()

set_target_properties(${TARGET_NAME} PROPERTIES EXPORT_NAME ris_framework)
if(NOT ANDROID)
    set_target_properties(${TARGET_NAME} PROPERTIES
            VERSION ${PROJECT_VERSION}
            SOVERSION ${PROJECT_VERSION_MAJOR})
endif()

# ---------------------------------------------------------------------------
# Samples and tests
# ---------------------------------------------------------------------------
if(RIS_FRAMEWORK_BUILD_SAMPLES)
    add_executable(ris_sample_c samples/sample_c.c)
    target_link_libraries(ris_sample_c PRIVATE ${TARGET_NAME})
    add_executable(ris_sample_cpp samples/sample_cpp.cpp)
    target_link_libraries(ris_sample_cpp PRIVATE ${TARGET_NAME})
    if(UNIX)
        target_link_libraries(ris_sample_c PRIVATE m)
    endif()
    # Ports of the ris-framework-ts / RisFramework.Demo examples. Assets are read from samples/<name>/assets.
    foreach(sample IN ITEMS sprite_batch unlit_mesh blinn_phong_mesh ktx2_quad ktx2_compare)
        add_executable(ris_sample_${sample} samples/${sample}/main.cpp)
        target_link_libraries(ris_sample_${sample} PRIVATE ${TARGET_NAME})
        target_compile_definitions(ris_sample_${sample} PRIVATE RIS_SAMPLES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/samples")
    endforeach()
endif()

if(RIS_FRAMEWORK_BUILD_TESTS)
    enable_testing()
    find_package(Catch2 CONFIG REQUIRED)
    file(GLOB TEST_SRC_FILES CONFIGURE_DEPENDS tests/*.cpp)
    add_executable(${TARGET_NAME}_tests ${TEST_SRC_FILES})
    target_link_libraries(${TARGET_NAME}_tests PRIVATE ${TARGET_NAME} Catch2::Catch2 Catch2::Catch2WithMain)
    target_compile_definitions(${TARGET_NAME}_tests PRIVATE RIS_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/data")
    add_test(NAME ${TARGET_NAME}_tests COMMAND ${TARGET_NAME}_tests)
endif()

# ---------------------------------------------------------------------------
# Install
# ---------------------------------------------------------------------------
if(RIS_FRAMEWORK_INSTALL)
    install(TARGETS ${TARGET_NAME}
            EXPORT ris_frameworkTargets
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

    install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp")

    install(EXPORT ris_frameworkTargets
            NAMESPACE ris_framework::
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ris_framework)

    configure_package_config_file(
            cmake/ris_frameworkConfig.cmake.in
            "${CMAKE_CURRENT_BINARY_DIR}/ris_frameworkConfig.cmake"
            INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ris_framework)
    write_basic_package_version_file(
            "${CMAKE_CURRENT_BINARY_DIR}/ris_frameworkConfigVersion.cmake"
            VERSION ${PROJECT_VERSION}
            COMPATIBILITY SameMajorVersion)
    install(FILES
            "${CMAKE_CURRENT_BINARY_DIR}/ris_frameworkConfig.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/ris_frameworkConfigVersion.cmake"
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ris_framework)

    install(FILES LICENSE THIRD_PARTY_NOTICES.md
            DESTINATION ${CMAKE_INSTALL_DATADIR}/ris_framework)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party")
        install(DIRECTORY third_party/ DESTINATION ${CMAKE_INSTALL_DATADIR}/ris_framework/third_party)
    endif()
endif()
