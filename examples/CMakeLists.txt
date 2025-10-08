# ----------------------------------------------------------------------
# Copyright 2025 Jody Hagins
# Distributed under the MIT Software License
# See accompanying file LICENSE or copy at
# https://opensource.org/licenses/MIT
# ----------------------------------------------------------------------

# Examples are designed to be standalone projects that fetch Atlas.
# We test them by building and running each as a separate project.

# Helper to create an example test
function(add_example_test name source_dir exec_name)
    set(build_dir ${CMAKE_CURRENT_BINARY_DIR}/${name}_build)

    add_test(NAME example_${name}
        COMMAND ${CMAKE_CTEST_COMMAND}
            --build-and-test
            ${CMAKE_CURRENT_SOURCE_DIR}/${source_dir}
            ${build_dir}
            --build-generator ${CMAKE_GENERATOR}
            --build-options
                -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
                -DFETCHCONTENT_BASE_DIR=${CMAKE_BINARY_DIR}/_deps
            --test-command ${exec_name}
    )

    set_tests_properties(example_${name} PROPERTIES
        TIMEOUT 300
        LABELS "examples"
        COST 100
    )
endfunction()

# Generate showcase examples automatically
set(SHOWCASE_OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/generated)
file(MAKE_DIRECTORY ${SHOWCASE_OUTPUT_DIR})

add_custom_command(
    OUTPUT ${SHOWCASE_OUTPUT_DIR}/strong_types_showcase.hpp
    COMMAND atlas
        --input=${CMAKE_CURRENT_SOURCE_DIR}/strong_types_showcase.txt
        --output=${SHOWCASE_OUTPUT_DIR}/strong_types_showcase.hpp
    DEPENDS atlas ${CMAKE_CURRENT_SOURCE_DIR}/strong_types_showcase.txt
    COMMENT "Generating strong_types_showcase.hpp"
    VERBATIM
)

add_custom_command(
    OUTPUT ${SHOWCASE_OUTPUT_DIR}/interactions_showcase.hpp
    COMMAND atlas
        --interactions=true
        --input=${CMAKE_CURRENT_SOURCE_DIR}/interactions_showcase.txt
        --output=${SHOWCASE_OUTPUT_DIR}/interactions_showcase.hpp
    DEPENDS atlas ${CMAKE_CURRENT_SOURCE_DIR}/interactions_showcase.txt
    COMMENT "Generating interactions_showcase.hpp"
    VERBATIM
)

add_custom_target(showcase_examples ALL
    DEPENDS
        ${SHOWCASE_OUTPUT_DIR}/strong_types_showcase.hpp
        ${SHOWCASE_OUTPUT_DIR}/interactions_showcase.hpp
    COMMENT "Building showcase examples"
)

# Add FetchContent example as integration test
# This verifies the primary user workflow: download Atlas via FetchContent and use it
# Other examples (basic, file, inline) remain as documentation but are not tested in CI
# Rationale: CMake helper variations are redundant with FetchContent test + compilation tests
add_example_test(fetchcontent fetchcontent_example example_app)
