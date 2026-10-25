# Eggs.SDL
#
# Copyright (c) 2026 Agustin Berge
#
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt

cmake_minimum_required(VERSION 4.0...4.4.0)
project(
    Eggs.SDL
    VERSION 1.0.0
    DESCRIPTION "A C++23 wrapper over SDL3"
    LANGUAGES CXX
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(GNUInstallDirs)

# Options (two spellings: short when top-level, prefixed when a subdirectory)
if(PROJECT_IS_TOP_LEVEL)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT ProgramDatabase)
    if(MSVC)
        string(APPEND CMAKE_EXE_LINKER_FLAGS " /DEBUG")
    endif()

    option(BUILD_DOCS "Build the documentation" ON)
    option(BUILD_EXAMPLES "Build the examples" ON)
    include(CTest)
    option(ENABLE_INSTALL "Generate install rules" ON)
else()
    set_property(DIRECTORY PROPERTY SYSTEM ON)

    option(EGGS_SDL_BUILD_DOCS "Build the documentation" OFF)
    option(EGGS_SDL_BUILD_EXAMPLES "Build the examples" OFF)
    option(EGGS_SDL_BUILD_TESTING "Build the test suite" OFF)
    option(EGGS_SDL_ENABLE_INSTALL "Generate install rules" OFF)

    set(BUILD_DOCS ${EGGS_SDL_BUILD_DOCS})
    set(BUILD_EXAMPLES ${EGGS_SDL_BUILD_EXAMPLES})
    set(BUILD_TESTING ${EGGS_SDL_BUILD_TESTING})
    set(ENABLE_INSTALL ${EGGS_SDL_ENABLE_INSTALL})
endif()

# SDL 3.2.0 is the first stable SDL3 release.
find_package(SDL3 3.2.0 CONFIG REQUIRED)

#
# Library target
#

add_library(_eggs_sdl INTERFACE)
target_compile_features(_eggs_sdl INTERFACE cxx_std_23)
# SDL3::SDL3 is an alias for the shared or the static target. Exporting it by
# name would resolve the alias and pin the package to the flavor that was
# installed here, so it is exported as an expression that the consumer resolves.
target_link_libraries(_eggs_sdl INTERFACE "$<TARGET_NAME_IF_EXISTS:SDL3::SDL3>")
target_sources(
    _eggs_sdl
    INTERFACE
        FILE_SET HEADERS
            BASE_DIRS include
            FILES
                include/eggs/sdl.hpp
                include/eggs/sdl/detail/bitmask.hpp
                include/eggs/sdl/detail/event_types.hpp
                include/eggs/sdl/detail/geometry.hpp
                include/eggs/sdl/detail/keycode.hpp
                include/eggs/sdl/detail/scancode.hpp
                include/eggs/sdl/error.hpp
                include/eggs/sdl/event.hpp
                include/eggs/sdl/init.hpp
                include/eggs/sdl/keyboard.hpp
                include/eggs/sdl/surface.hpp
                include/eggs/sdl/window.hpp
)
set_target_properties(
    _eggs_sdl
    PROPERTIES EXPORT_NAME SDL VERIFY_INTERFACE_HEADER_SETS ${BUILD_TESTING}
)
add_library(Eggs::SDL ALIAS _eggs_sdl)

#
# Documentation
#
if(BUILD_DOCS)
    add_subdirectory(docs)
endif()

#
# Examples
#
if(BUILD_EXAMPLES)
    add_subdirectory(example)
endif()

#
# Tests
#
if(BUILD_TESTING)
    add_subdirectory(test)
endif()

#
# Install rules
#
if(ENABLE_INSTALL)
    include(CMakePackageConfigHelpers)

    install(
        TARGETS _eggs_sdl
        EXPORT EggsSDLTargets
        FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
    install(
        EXPORT EggsSDLTargets
        NAMESPACE Eggs::
        FILE eggs.sdl-targets.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/eggs.sdl
    )

    configure_package_config_file(
        cmake/eggs.sdl-config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/eggs.sdl-config.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/eggs.sdl
    )

    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/eggs.sdl-config-version.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
        ARCH_INDEPENDENT
    )

    install(
        FILES
            ${CMAKE_CURRENT_BINARY_DIR}/eggs.sdl-config.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/eggs.sdl-config-version.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/eggs.sdl
    )

    set(CPACK_PACKAGE_NAME "Eggs.SDL")
    set(CPACK_PACKAGE_VENDOR "Agustin Berge")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
    if(WIN32)
        set(CPACK_GENERATOR ZIP)
    else()
        set(CPACK_GENERATOR TGZ)
    endif()
    include(CPack)
endif()
