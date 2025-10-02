# ----------------------------------------------------------------------
# Copyright 2025 Jody Hagins
# Distributed under the MIT Software License
# See accompanying file LICENSE or copy at
# https://opensource.org/licenses/MIT
# ----------------------------------------------------------------------
#
# Compiler Warning Configuration
#
# This module sets up comprehensive compiler warnings for Atlas.
# Warnings are treated as errors by default to maintain code quality.
#
# Usage:
#   include(cmake/CompilerWarnings.cmake)
#   target_apply_atlas_warnings(target_name)
#
# Options:
#   ATLAS_WARNINGS_AS_ERRORS - Treat warnings as errors (default: ON)
#

option(ATLAS_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)

function(target_apply_atlas_warnings target_name)
    set(warning_flags "")

    if(MSVC)
        list(APPEND warning_flags
            /W4                     # Enable most warnings
            /permissive-            # Conformance mode
            /w14242                 # Conversion warnings
            /w14254                 # Operator mismatch warnings
            /w14263                 # Member function override
            /w14265                 # Virtual destructor warnings
            /w14287                 # Unsigned/negative constant mismatch
            /we4289                 # Loop variable used outside scope (error)
            /w14296                 # Expression always true/false
            /w14311                 # Pointer truncation
            /w14545                 # Expression before comma missing
            /w14546                 # Function call before comma missing
            /w14547                 # Operator before comma has no effect
            /w14549                 # Operator before comma has no effect (2)
            /w14555                 # Expression has no effect
            /w14619                 # Pragma warning suppression
            /w14640                 # Thread-safe static initialization
            /w14826                 # Conversion sign extension
            /w14905                 # String literal to LPSTR
            /w14906                 # Wide string to LPSTR
            /w14928                 # Illegal copy-initialization
            /w14996                 # Deprecated function usage
        )

        if(ATLAS_WARNINGS_AS_ERRORS)
            list(APPEND warning_flags /WX)
        endif()

    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        list(APPEND warning_flags
            # Core warnings
            -Wall
            -Wextra
            -Wpedantic

            # Highly recommended
            -Wshadow                    # Variable shadowing
            -Wnon-virtual-dtor          # Non-virtual destructor
            -Wold-style-cast            # C-style casts
            -Wcast-align                # Alignment issues
            -Wunused                    # Unused variables/functions
            -Woverloaded-virtual        # Overloaded virtual functions
            -Wconversion                # Type conversions
            -Wsign-conversion           # Sign conversions
            -Wnull-dereference          # Null pointer dereference
            -Wdouble-promotion          # Float to double promotion
            -Wformat=2                  # Format string vulnerabilities

            # Good to have
            -Wmisleading-indentation    # Misleading indentation
            -Wduplicated-cond           # Duplicated if-else conditions
            -Wduplicated-branches       # Duplicated branches
            -Wlogical-op                # Logical operation issues
            -Wuseless-cast              # Unnecessary casts
        )

        # GCC 8+ specific warnings
        if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 8.0)
            list(APPEND warning_flags
                -Wcast-function-type
                -Wstringop-overflow=4
            )
        endif()

        # GCC 10+ specific warnings
        if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 10.0)
            list(APPEND warning_flags
                -Warith-conversion
            )
        endif()

        if(ATLAS_WARNINGS_AS_ERRORS)
            list(APPEND warning_flags -Werror)
        endif()

    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        list(APPEND warning_flags
            # Core warnings
            -Wall
            -Wextra
            -Wpedantic

            # Highly recommended
            -Wshadow-all                # All shadowing (stricter than -Wshadow)
            -Wnon-virtual-dtor
            -Wold-style-cast
            -Wcast-align
            -Wunused
            -Woverloaded-virtual
            -Wconversion
            -Wsign-conversion
            -Wnull-dereference
            -Wdouble-promotion
            -Wformat=2

            # Clang-specific
            -Wimplicit-fallthrough      # Missing fallthrough annotations
            -Wmost                      # Most common issues
            -Wextra-semi                # Extra semicolons
            -Wunreachable-code          # Unreachable code
            -Wstrict-aliasing           # Aliasing violations

            # Modern C++ specific
            -Winconsistent-missing-destructor-override
            -Wcovered-switch-default    # Default in complete switch

            # Compatibility disables (as needed)
            -Wno-gnu-zero-variadic-macro-arguments  # For Boost Describe
        )

        if(ATLAS_WARNINGS_AS_ERRORS)
            list(APPEND warning_flags -Werror)
        endif()
    endif()

    target_compile_options(${target_name} PRIVATE ${warning_flags})

    # Print applied warnings for debugging
    if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND CMAKE_VERBOSE_MAKEFILE)
        message(STATUS "Applied warnings to ${target_name}: ${warning_flags}")
    endif()
endfunction()
