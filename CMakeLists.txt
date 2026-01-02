# ----------------------------------------------------------------------
# Copyright 2025 Jody Hagins
# Distributed under the MIT Software License
# See accompanying file LICENSE or copy at
# https://opensource.org/licenses/MIT
# ----------------------------------------------------------------------
cmake_minimum_required(VERSION 3.20)

project(Atlas
    VERSION 1.0.0
    LANGUAGES CXX)

# Generate version header from template (single source of truth for version)
set(ATLAS_GENERATED_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated
    CACHE INTERNAL "Directory containing generated Atlas headers")
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.hpp.in
    ${ATLAS_GENERATED_INCLUDE_DIR}/atlas/version.hpp
    @ONLY)

# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release"
        CACHE STRING "Choose the type of build."
        FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE
        PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
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
option(ENABLE_COVERAGE "Enable coverage reporting" OFF)

# Coverage support (must be set before adding subdirectories/targets)
if(ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(STATUS "Enabling coverage support")
        add_compile_options(--coverage -fprofile-arcs -ftest-coverage)
        add_link_options(--coverage)
    endif()
endif()

if(USE_SYSTEM_BOOST)
    find_package(Boost
        REQUIRED
        COMPONENTS json)
else()
    # Fetch Boost from release tarball (much faster than git clone)
    fetchcontent_declare(Boost
        URL https://github.com/boostorg/boost/releases/download/boost-1.85.0/boost-1.85.0-cmake.tar.gz
        URL_HASH SHA256=ab9c9c4797384b0949dd676cf86b4f99553f8c148d767485aaac412af25183e6
        SYSTEM
        DOWNLOAD_EXTRACT_TIMESTAMP ON)

    # Configure Boost to only build what we need
    set(BOOST_INCLUDE_LIBRARIES json describe uuid headers)
    set(BOOST_ENABLE_CMAKE ON)

    fetchcontent_makeavailable(Boost)
endif()

# Fetch boost.mustache
fetchcontent_declare(boost_mustache
    GIT_REPOSITORY https://github.com/pdimov/mustache.git
    GIT_TAG develop
    GIT_SHALLOW ON
    GIT_PROGRESS ON
    SYSTEM)

fetchcontent_makeavailable(boost_mustache)

# Add src/lib subdirectory which creates atlas_lib
add_subdirectory(src/lib/atlas)

add_executable(atlas src/tools/atlas.cpp)

# Create an alias target for consistent usage in both build and install trees
add_executable(Atlas::atlas ALIAS atlas)

target_link_libraries(atlas
    PRIVATE
        atlas_lib)

# Apply compiler warnings to executable
target_apply_atlas_warnings(atlas)

# Export the atlas executable location for downstream users
set(Atlas_EXECUTABLE $<TARGET_FILE:atlas>
    CACHE INTERNAL "Path to atlas executable")

# Tests
if(ATLAS_BUILD_TESTS AND CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    enable_testing()
    add_subdirectory(tests)
    add_subdirectory(examples)
endif()

# Installation rules
include(CMakePackageConfigHelpers)

# Install the executable (atlas_lib is an internal implementation detail and not exported)
install(TARGETS atlas
    EXPORT AtlasTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# Note: atlas_lib is NOT exported or installed
# The library uses Boost internally which cannot be reliably exported from FetchContent
# Users should use the atlas executable tool, not link against the library

# Headers are not installed since atlas_lib is not exported
# Users should use the atlas executable tool to generate code

# Export targets for find_package
install(EXPORT AtlasTargets
    FILE AtlasTargets.cmake
    NAMESPACE Atlas::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Atlas)

# Create config file
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/AtlasConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/AtlasConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Atlas)

# Create version file
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/AtlasConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)

# Install config files
install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/AtlasConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/AtlasConfigVersion.cmake
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/AtlasHelpers.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Atlas)

# Export targets for use in build tree (for FetchContent users)
export(EXPORT AtlasTargets
    FILE ${CMAKE_CURRENT_BINARY_DIR}/AtlasTargets.cmake
    NAMESPACE Atlas::)

# Copy helper functions to build tree for FetchContent users
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/AtlasHelpers.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/AtlasHelpers.cmake
    COPYONLY)

# Include helpers directly for FetchContent users (they don't use the config file)
# When Atlas is used via add_subdirectory or FetchContent, include helpers automatically
if(NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    # Atlas is being included as a subdirectory/FetchContent - make helpers available
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/AtlasHelpers.cmake)
endif()

# Add custom coverage target
if(ATLAS_BUILD_TESTS)
    find_program(LCOV_PATH lcov)
    find_program(GENHTML_PATH genhtml)

    # Try to find llvm-cov for better branch coverage on Clang
    # Use wrapper script if available, otherwise try to find llvm-cov directly
    set(LLVM_GCOV_WRAPPER "${CMAKE_SOURCE_DIR}/scripts/llvm-gcov.sh")
    if(EXISTS ${LLVM_GCOV_WRAPPER} AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(GCOV_TOOL ${LLVM_GCOV_WRAPPER})
        message(STATUS "Using llvm-cov via wrapper for coverage (better branch coverage support)")
    else()
        set(GCOV_TOOL gcov)
    endif()

    if(LCOV_PATH AND GENHTML_PATH)
        add_custom_target(coverage
            COMMAND ${CMAKE_COMMAND} -E echo "Cleaning dependency coverage data..."
            COMMAND find ${CMAKE_BINARY_DIR} -name "*.gcda" -path "*/_deps/*" -delete
            COMMAND ${CMAKE_COMMAND} -E echo "Running tests for coverage..."
            COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
            COMMAND ${CMAKE_COMMAND} -E echo "Collecting coverage data..."
            COMMAND ${LCOV_PATH} --capture --directory ${CMAKE_BINARY_DIR} --output-file coverage_raw.info
                --rc branch_coverage=1 --rc derive_function_end_line=0
                --exclude '*/_deps/*' --exclude '*/build/_deps/*'
                --ignore-errors mismatch,source,unused --gcov-tool ${GCOV_TOOL}
            COMMAND ${CMAKE_COMMAND} -E echo "Filtering to project sources only..."
            COMMAND ${LCOV_PATH} --remove coverage_raw.info '/usr/*' '*/tests/*' '*/_deps/*' '*/doctest.hpp'
                --output-file coverage.info --rc branch_coverage=1 --rc derive_function_end_line=0
                --ignore-errors unused
            COMMAND ${CMAKE_COMMAND} -E echo "Generating HTML report..."
            COMMAND ${GENHTML_PATH} coverage.info --output-directory coverage_html --rc branch_coverage=1
                --ignore-errors source,inconsistent --title "Atlas Coverage Report" --legend
            COMMAND ${CMAKE_COMMAND} -E echo ""
            COMMAND ${CMAKE_COMMAND} -E echo "========================================"
            COMMAND ${CMAKE_COMMAND} -E echo "Coverage report generated successfully!"
            COMMAND ${CMAKE_COMMAND} -E echo ""
            COMMAND ${CMAKE_COMMAND} -E echo "View the report:"
            COMMAND ${CMAKE_COMMAND} -E echo "  open ${CMAKE_BINARY_DIR}/coverage_html/index.html"
            COMMAND ${CMAKE_COMMAND} -E echo ""
            COMMAND ${CMAKE_COMMAND} -E echo "Or run: scripts/show_coverage.sh"
            COMMAND ${CMAKE_COMMAND} -E echo "========================================"
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Generating coverage report")
    else()
        add_custom_target(coverage
            COMMAND ${CMAKE_COMMAND} -E echo "Running tests..."
            COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
            COMMAND ${CMAKE_COMMAND} -E echo "Generating basic coverage report..."
            COMMAND ${CMAKE_COMMAND} -E echo
                "Install lcov for HTML reports: brew install lcov (macOS) or apt-get install lcov (Linux)"
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Running tests (lcov not found for detailed coverage)")
    endif()
endif()
