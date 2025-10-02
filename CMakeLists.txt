# ----------------------------------------------------------------------
# Copyright 2025 Jody Hagins
# Distributed under the MIT Software License
# See accompanying file LICENSE or copy at
# https://opensource.org/licenses/MIT
# ----------------------------------------------------------------------
cmake_minimum_required(VERSION 3.20)
project(Atlas VERSION 1.0.0 LANGUAGES CXX)

# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set output directories for all targets
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Include standard installation directories
include(GNUInstallDirs)

# Include compiler warning configuration
include(cmake/CompilerWarnings.cmake)

# Include FetchContent module
include(FetchContent)

# Options
option(USE_SYSTEM_BOOST "Use system Boost installation" OFF)
option(ATLAS_BUILD_TESTS "Build Atlas tests (requires CMake 3 for doctest)" ON)

if(USE_SYSTEM_BOOST)
    find_package(Boost REQUIRED COMPONENTS json)
else()
    # Fetch Boost from release tarball (much faster than git clone)
    FetchContent_Declare(
        Boost
        URL https://github.com/boostorg/boost/releases/download/boost-1.85.0/boost-1.85.0-cmake.tar.gz
        URL_HASH SHA256=ab9c9c4797384b0949dd676cf86b4f99553f8c148d767485aaac412af25183e6
        SYSTEM
        DOWNLOAD_EXTRACT_TIMESTAMP ON
    )

    # Configure Boost to only build what we need
    set(BOOST_INCLUDE_LIBRARIES json describe uuid headers)
    set(BOOST_ENABLE_CMAKE ON)

    FetchContent_MakeAvailable(Boost)
endif()

# Fetch boost.mustache
FetchContent_Declare(
    boost_mustache
    GIT_REPOSITORY https://github.com/pdimov/mustache.git
    GIT_TAG develop
    GIT_SHALLOW ON
    GIT_PROGRESS ON
    SYSTEM
)
FetchContent_MakeAvailable(boost_mustache)

add_library(atlas_lib
    src/lib/StrongTypeGenerator.cpp
    src/lib/StrongTypeGenerator.hpp
    src/lib/InteractionGenerator.cpp
    src/lib/InteractionGenerator.hpp
    src/lib/AtlasUtilities.cpp
    src/lib/AtlasUtilities.hpp
    src/lib/AtlasCommandLine.cpp
    src/lib/AtlasCommandLine.hpp
)

target_include_directories(atlas_lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/lib>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/atlas>
)

target_link_libraries(atlas_lib
    PRIVATE
        Boost::describe
        Boost::json
        Boost::uuid
)

# Apply compiler warnings to library
target_apply_atlas_warnings(atlas_lib)

# Add boost and mustache include directories
target_include_directories(atlas_lib
    SYSTEM PRIVATE
        ${boost_mustache_SOURCE_DIR}/include
        ${boost_SOURCE_DIR}
)

# Add mustache source file to the library
target_sources(atlas_lib
    PRIVATE
        ${boost_mustache_SOURCE_DIR}/src/renderer.cpp
)

# Disable strict warnings for third-party mustache source
set_source_files_properties(
    ${boost_mustache_SOURCE_DIR}/src/renderer.cpp
    PROPERTIES
        COMPILE_FLAGS "-Wno-covered-switch-default -Wno-sign-conversion"
)

add_executable(atlas
    src/tools/atlas.cpp
)

# Create an alias target for consistent usage in both build and install trees
add_executable(Atlas::atlas ALIAS atlas)

target_link_libraries(atlas
    PRIVATE
        atlas_lib
)

# Apply compiler warnings to executable
target_apply_atlas_warnings(atlas)

# Export the atlas executable location for downstream users
set(Atlas_EXECUTABLE $<TARGET_FILE:atlas> CACHE INTERNAL "Path to atlas executable")

# Tests
if (ATLAS_BUILD_TESTS AND CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    enable_testing()
    add_subdirectory(tests)
endif()

# Installation rules
include(CMakePackageConfigHelpers)

# Install the executable (atlas_lib is an internal implementation detail and not exported)
install(TARGETS atlas
    EXPORT AtlasTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Note: atlas_lib is NOT exported or installed
# The library uses Boost internally which cannot be reliably exported from FetchContent
# Users should use the atlas executable tool, not link against the library

# Headers are not installed since atlas_lib is not exported
# Users should use the atlas executable tool to generate code

# Export targets for find_package
install(EXPORT AtlasTargets
    FILE AtlasTargets.cmake
    NAMESPACE Atlas::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Atlas
)

# Create config file
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/AtlasConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/AtlasConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Atlas
)

# Create version file
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/AtlasConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Install config files
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/AtlasConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/AtlasConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Atlas
)

# Export targets for use in build tree (for FetchContent users)
export(EXPORT AtlasTargets
    FILE ${CMAKE_CURRENT_BINARY_DIR}/AtlasTargets.cmake
    NAMESPACE Atlas::
)

