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
            --test-command ${exec_name}
    )

    set_tests_properties(example_${name} PROPERTIES
        TIMEOUT 300
        LABELS "examples"
    )
endfunction()

# Add each example as a test
add_example_test(fetchcontent fetchcontent_example example_app)
add_example_test(basic helpers_basic_example basic_example)
add_example_test(file helpers_file_example file_example)
add_example_test(inline helpers_inline_example inline_example)
