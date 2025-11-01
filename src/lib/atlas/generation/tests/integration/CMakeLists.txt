# ----------------------------------------------------------------------
# Copyright 2025 Jody Hagins
# Distributed under the MIT Software License
# See accompanying file LICENSE or copy at
# https://opensource.org/licenses/MIT
# ----------------------------------------------------------------------

# Integration test for the entire code generation pipeline
add_executable(GenerationPipeline_ut GenerationPipeline_ut.cpp)

# Include the project test directory for doctest.hpp wrapper
target_include_directories(GenerationPipeline_ut
    PRIVATE
        ${CMAKE_SOURCE_DIR})  # For doctest.hpp wrapper

# Link with required libraries
# Integration tests need the complete atlas_lib and generation library
target_link_libraries(GenerationPipeline_ut
    PRIVATE
        atlas_lib       # Full StrongTypeGenerator API
        generation      # Complete generation library with all templates
        doctest::doctest)

# Force-link the generation library to ensure static initializers run
# This is critical for template self-registration to work correctly.
if(APPLE)
    # macOS: Use -force_load
    target_link_options(GenerationPipeline_ut
        PRIVATE
            "LINKER:-force_load,$<TARGET_FILE:generation>")
elseif(UNIX)
    # Linux: Use --whole-archive
    target_link_options(GenerationPipeline_ut
        PRIVATE
            "LINKER:--whole-archive"
            "$<TARGET_FILE:generation>"
            "LINKER:--no-whole-archive")
elseif(MSVC)
    # Windows: Use /WHOLEARCHIVE
    target_link_options(GenerationPipeline_ut
        PRIVATE
            "/WHOLEARCHIVE:$<TARGET_FILE:generation>")
endif()

# Enable testing
add_test(NAME GenerationPipelineIntegrationTests COMMAND GenerationPipeline_ut)

# Set output directory to match other tests
set_target_properties(GenerationPipeline_ut PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
