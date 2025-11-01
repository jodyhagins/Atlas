# ----------------------------------------------------------------------
# Copyright 2025 Jody Hagins
# Distributed under the MIT Software License
# See accompanying file LICENSE or copy at
# https://opensource.org/licenses/MIT
# ----------------------------------------------------------------------

# Test executable for TemplateRegistry
add_executable(TemplateRegistry_ut TemplateRegistry_ut.cpp)

# Include the project test directory for doctest.hpp wrapper
target_include_directories(TemplateRegistry_ut
    PRIVATE
        ${CMAKE_SOURCE_DIR})  # For doctest.hpp wrapper

# Link with required libraries
# TemplateRegistry tests need atlas_lib for parse() function
target_link_libraries(TemplateRegistry_ut
    PRIVATE
        atlas_lib       # Provides parse() from StrongTypeGenerator
        generation      # Link to generation library (brings in Boost::json as PUBLIC dependency)
        doctest::doctest)

# Enable testing
add_test(NAME TemplateRegistryTests COMMAND TemplateRegistry_ut)

# Set output directory to match other tests
set_target_properties(TemplateRegistry_ut PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Test executable for GuardGenerator
add_executable(GuardGenerator_ut GuardGenerator_ut.cpp)

# Include the project test directory for doctest.hpp wrapper
target_include_directories(GuardGenerator_ut
    PRIVATE
        ${CMAKE_SOURCE_DIR})  # For doctest.hpp wrapper

# Link with required libraries
# GuardGenerator needs atlas_lib for get_sha1() function
target_link_libraries(GuardGenerator_ut
    PRIVATE
        atlas_lib       # Provides get_sha1() via AtlasUtilities
        generation      # Link to generation library
        doctest::doctest)

# Enable testing
add_test(NAME GuardGeneratorTests COMMAND GuardGenerator_ut)

# Set output directory to match other tests
set_target_properties(GuardGenerator_ut PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Test executable for MainTemplate
add_executable(MainTemplate_ut MainTemplate_ut.cpp)

# Include the project test directory for doctest.hpp wrapper
target_include_directories(MainTemplate_ut
    PRIVATE
        ${CMAKE_SOURCE_DIR})  # For doctest.hpp wrapper

# Link with required libraries
# MainTemplate needs atlas_lib for parse() function
target_link_libraries(MainTemplate_ut
    PRIVATE
        atlas_lib       # Provides parse() from StrongTypeGenerator
        generation      # Link to generation library (brings in MainTemplate and dependencies)
        doctest::doctest)

# Enable testing
add_test(NAME MainTemplateTests COMMAND MainTemplate_ut)

# Set output directory to match other tests
set_target_properties(MainTemplate_ut PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Test executable for TemplateOrchestrator
add_executable(TemplateOrchestrator_ut TemplateOrchestrator_ut.cpp)

# Include the project test directory for doctest.hpp wrapper
target_include_directories(TemplateOrchestrator_ut
    PRIVATE
        ${CMAKE_SOURCE_DIR})  # For doctest.hpp wrapper

# Link with required libraries
# TemplateOrchestrator needs atlas_lib for parse() and the full generation library
target_link_libraries(TemplateOrchestrator_ut
    PRIVATE
        atlas_lib       # Provides parse() from StrongTypeGenerator
        generation      # Link to generation library (brings in TemplateOrchestrator and dependencies)
        doctest::doctest)

# Enable testing
add_test(NAME TemplateOrchestratorTests COMMAND TemplateOrchestrator_ut)

# Set output directory to match other tests
set_target_properties(TemplateOrchestrator_ut PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
