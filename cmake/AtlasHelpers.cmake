# ----------------------------------------------------------------------
# Copyright 2025 Jody Hagins
# Distributed under the MIT Software License
# See accompanying file LICENSE or copy at
# https://opensource.org/licenses/MIT
# ----------------------------------------------------------------------

# Atlas Strong Type CMake Helper Functions
#
# These functions simplify the integration of Atlas Strong Type Generator
# into CMake-based projects. They handle code generation, dependency
# management, and build integration automatically.

# Internal helper function to deduce namespace from source directory path
# Converts directory structure under PROJECT_SOURCE_DIR/src to C++ namespace
# Example: /path/to/project/src/foo/bar -> foo::bar
function(_atlas_deduce_namespace OUT_VAR)
    # Get the relative path from PROJECT_SOURCE_DIR/src to CMAKE_CURRENT_SOURCE_DIR
    if(EXISTS "${PROJECT_SOURCE_DIR}/src")
        file(RELATIVE_PATH rel_path "${PROJECT_SOURCE_DIR}/src" "${CMAKE_CURRENT_SOURCE_DIR}")

        # If we're inside src/, convert path separators to ::
        if(NOT rel_path MATCHES "^\\.\\.")
            string(REPLACE "/" "::" namespace "${rel_path}")
            set(${OUT_VAR} "${namespace}" PARENT_SCOPE)
            return()
        endif()
    endif()

    # Fallback: use project name as namespace
    string(TOLOWER "${PROJECT_NAME}" namespace)
    set(${OUT_VAR} "${namespace}" PARENT_SCOPE)
endfunction()

# Add a single Atlas-generated strong type
#
# This function generates a C++ strong type wrapper using Atlas and integrates
# it into the build system. The generated file is placed in the source tree
# for version control and IDE navigation.
#
# Parameters:
#   NAME - Required: Name of the strong type (e.g., UserId, Distance)
#   TYPE - Required: Underlying C++ type to wrap (e.g., int, double, size_t)
#   DESCRIPTION - Required: Atlas operators/features (e.g., "==, !=, <=>")
#   NAMESPACE - Optional: C++ namespace (defaults to auto-deduced from directory)
#   KIND - Optional: 'struct' or 'class' (defaults to 'struct')
#   DEFAULT_VALUE - Optional: Default value for default constructor
#   GUARD_PREFIX - Optional: Custom prefix for header guards
#   GUARD_SEPARATOR - Optional: Separator for header guard components
#   UPCASE_GUARD - Optional: Use uppercase header guards (true/false)
#   OUTPUT - Optional: Output file path (defaults to ${CMAKE_CURRENT_SOURCE_DIR}/${NAME}.hpp)
#   TARGET - Optional: Target to add dependency to (if not specified, no target integration)
#
# Examples:
#   # Minimal usage - namespace auto-deduced from directory path
#   add_atlas_strong_type(
#       NAME UserId
#       TYPE int
#       DESCRIPTION "==, !=, <=>")
#
#   # With custom target and parameters
#   add_atlas_strong_type(
#       NAME Distance
#       TYPE double
#       DESCRIPTION "+, -, *, /, <=>"
#       TARGET my_library
#       DEFAULT_VALUE 0.0)
#
function(add_atlas_strong_type)
    set(options "")
    set(oneValueArgs
        NAME
        TYPE
        DESCRIPTION
        NAMESPACE
        KIND
        DEFAULT_VALUE
        GUARD_PREFIX
        GUARD_SEPARATOR
        UPCASE_GUARD
        OUTPUT
        TARGET)
    set(multiValueArgs "")
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Validate required arguments
    if(NOT ARG_NAME)
        message(FATAL_ERROR "add_atlas_strong_type: NAME is required")
    endif()
    if(NOT ARG_TYPE)
        message(FATAL_ERROR "add_atlas_strong_type: TYPE is required")
    endif()
    if(NOT ARG_DESCRIPTION)
        message(FATAL_ERROR "add_atlas_strong_type: DESCRIPTION is required")
    endif()

    # Set defaults
    if(NOT ARG_NAMESPACE)
        _atlas_deduce_namespace(ARG_NAMESPACE)
    endif()

    if(NOT ARG_KIND)
        set(ARG_KIND "struct")
    endif()

    if(NOT ARG_OUTPUT)
        set(ARG_OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/${ARG_NAME}.hpp")
    endif()

    # Build description with proper escaping (semicolons are CMake list separators)
    string(REPLACE ";" "\\;" ESCAPED_DESCRIPTION "${ARG_DESCRIPTION}")
    set(FULL_DESCRIPTION "strong ${ARG_TYPE}\\; ${ESCAPED_DESCRIPTION}")

    # Build Atlas command arguments
    set(ATLAS_ARGS "")
    list(APPEND ATLAS_ARGS "--kind=${ARG_KIND}")
    list(APPEND ATLAS_ARGS "--namespace=${ARG_NAMESPACE}")
    list(APPEND ATLAS_ARGS "--name=${ARG_NAME}")
    list(APPEND ATLAS_ARGS "--description=${FULL_DESCRIPTION}")
    list(APPEND ATLAS_ARGS "--output=${ARG_OUTPUT}")

    # Add optional parameters only if set
    if(ARG_DEFAULT_VALUE)
        list(APPEND ATLAS_ARGS "--default-value=${ARG_DEFAULT_VALUE}")
    endif()

    if(ARG_GUARD_PREFIX)
        list(APPEND ATLAS_ARGS "--guard-prefix=${ARG_GUARD_PREFIX}")
    endif()

    if(ARG_GUARD_SEPARATOR)
        list(APPEND ATLAS_ARGS "--guard-separator=${ARG_GUARD_SEPARATOR}")
    endif()

    if(DEFINED ARG_UPCASE_GUARD)
        list(APPEND ATLAS_ARGS "--upcase-guard=${ARG_UPCASE_GUARD}")
    endif()

    # Determine Atlas executable location
    if(TARGET Atlas::atlas)
        set(ATLAS_EXECUTABLE $<TARGET_FILE:Atlas::atlas>)
    elseif(DEFINED Atlas_EXECUTABLE)
        set(ATLAS_EXECUTABLE ${Atlas_EXECUTABLE})
    else()
        message(
            FATAL_ERROR
                "add_atlas_strong_type: Atlas executable not found. Ensure Atlas is available via find_package or FetchContent."
        )
    endif()

    # Create custom command to generate the file
    add_custom_command(
        OUTPUT ${ARG_OUTPUT}
        COMMAND ${ATLAS_EXECUTABLE} ${ATLAS_ARGS}
        DEPENDS ${ATLAS_EXECUTABLE}
        COMMENT "Generating ${ARG_NAME}.hpp with Atlas"
        VERBATIM)

    # Create custom target for this generation
    set(target_name "generate_${ARG_NAME}")
    add_custom_target(${target_name} DEPENDS ${ARG_OUTPUT})

    # Add dependency to the specified target (if provided)
    if(ARG_TARGET)
        if(TARGET ${ARG_TARGET})
            add_dependencies(${ARG_TARGET} ${target_name})
            target_sources(${ARG_TARGET} PUBLIC ${ARG_OUTPUT})
        else()
            message(WARNING "add_atlas_strong_type: Target '${ARG_TARGET}' does not exist")
        endif()
    endif()
endfunction()

# Add multiple Atlas-generated strong types from an input file
#
# This function generates multiple C++ strong types from a configuration file
# and produces a single output header containing all types.
#
# Parameters:
#   INPUT - Required: Input file containing type definitions
#   OUTPUT - Required: Output file path for generated code
#   TARGET - Optional: Target to add dependency to (if not specified, no target integration)
#
# Input File Format:
#   # Optional file-level configuration
#   guard_prefix=MY_TYPES
#   guard_separator=_
#   upcase_guard=true
#
#   # Type definitions
#   [type]
#   kind=struct
#   namespace=example
#   name=Distance
#   description=strong double; +, -, *, /, <=>
#   default_value=0.0
#
#   [type]
#   kind=struct
#   namespace=example
#   name=Handle
#   description=strong size_t; ->, ==, !=, bool
#
# Example:
#   add_atlas_strong_types_from_file(
#       INPUT strong_types.txt
#       OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/GeneratedTypes.hpp
#       TARGET my_library)
#
function(add_atlas_strong_types_from_file)
    set(options "")
    set(oneValueArgs INPUT OUTPUT TARGET)
    set(multiValueArgs "")
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Validate required arguments
    if(NOT ARG_INPUT)
        message(FATAL_ERROR "add_atlas_strong_types_from_file: INPUT is required")
    endif()
    if(NOT ARG_OUTPUT)
        message(FATAL_ERROR "add_atlas_strong_types_from_file: OUTPUT is required")
    endif()

    # Make input path absolute if relative
    if(NOT IS_ABSOLUTE "${ARG_INPUT}")
        set(ARG_INPUT "${CMAKE_CURRENT_SOURCE_DIR}/${ARG_INPUT}")
    endif()

    # Determine Atlas executable location
    if(TARGET Atlas::atlas)
        set(ATLAS_EXECUTABLE $<TARGET_FILE:Atlas::atlas>)
    elseif(DEFINED Atlas_EXECUTABLE)
        set(ATLAS_EXECUTABLE ${Atlas_EXECUTABLE})
    else()
        message(
            FATAL_ERROR
                "add_atlas_strong_types_from_file: Atlas executable not found. Ensure Atlas is available via find_package or FetchContent."
        )
    endif()

    # Create custom command to generate from file
    add_custom_command(
        OUTPUT ${ARG_OUTPUT}
        COMMAND ${ATLAS_EXECUTABLE} --input=${ARG_INPUT} --output=${ARG_OUTPUT}
        DEPENDS ${ATLAS_EXECUTABLE} ${ARG_INPUT}
        COMMENT "Generating strong types from ${ARG_INPUT} with Atlas"
        VERBATIM)

    # Create custom target for this generation
    get_filename_component(output_name ${ARG_OUTPUT} NAME_WE)
    set(target_name "generate_${output_name}")
    add_custom_target(${target_name} DEPENDS ${ARG_OUTPUT})

    # Add dependency to the specified target (if provided)
    if(ARG_TARGET)
        if(TARGET ${ARG_TARGET})
            add_dependencies(${ARG_TARGET} ${target_name})
            target_sources(${ARG_TARGET} PUBLIC ${ARG_OUTPUT})
        else()
            message(WARNING "add_atlas_strong_types_from_file: Target '${ARG_TARGET}' does not exist")
        endif()
    endif()
endfunction()

# Add multiple Atlas-generated strong types defined inline in CMake
#
# This function allows you to define strong types directly in your CMakeLists.txt
# without needing a separate input file. Useful for simple projects or when you
# want to keep type definitions close to where they're used.
#
# Parameters:
#   OUTPUT - Required: Output file path for generated code
#   CONTENT - Required: Type definitions in Atlas input file format
#   TARGET - Optional: Target to add dependency to (if not specified, no target integration)
#
# Example:
#   add_atlas_strong_types_inline(
#       OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/Types.hpp
#       TARGET my_library
#       CONTENT "
#   guard_prefix=MY_TYPES
#
#   [type]
#   kind=struct
#   namespace=example
#   name=UserId
#   description=strong int; ==, !=, <=>
#
#   [type]
#   kind=struct
#   namespace=example
#   name=Distance
#   description=strong double; +, -, *, /, <=>
#   ")
#
function(add_atlas_strong_types_inline)
    set(options "")
    set(oneValueArgs OUTPUT CONTENT TARGET)
    set(multiValueArgs "")
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Validate required arguments
    if(NOT ARG_OUTPUT)
        message(FATAL_ERROR "add_atlas_strong_types_inline: OUTPUT is required")
    endif()
    if(NOT ARG_CONTENT)
        message(FATAL_ERROR "add_atlas_strong_types_inline: CONTENT is required")
    endif()

    # Create a unique temporary input file in the build directory
    get_filename_component(output_name ${ARG_OUTPUT} NAME_WE)
    set(TEMP_INPUT "${CMAKE_CURRENT_BINARY_DIR}/.atlas_inline_${output_name}.txt")

    # Write content to temporary file
    file(WRITE ${TEMP_INPUT} "${ARG_CONTENT}")

    # Determine Atlas executable location
    if(TARGET Atlas::atlas)
        set(ATLAS_EXECUTABLE $<TARGET_FILE:Atlas::atlas>)
    elseif(DEFINED Atlas_EXECUTABLE)
        set(ATLAS_EXECUTABLE ${Atlas_EXECUTABLE})
    else()
        message(
            FATAL_ERROR
                "add_atlas_strong_types_inline: Atlas executable not found. Ensure Atlas is available via find_package or FetchContent."
        )
    endif()

    # Create custom command to generate from inline content
    add_custom_command(
        OUTPUT ${ARG_OUTPUT}
        COMMAND ${ATLAS_EXECUTABLE} --input=${TEMP_INPUT} --output=${ARG_OUTPUT}
        DEPENDS ${ATLAS_EXECUTABLE} ${TEMP_INPUT}
        COMMENT "Generating strong types from inline content with Atlas"
        VERBATIM)

    # Create custom target for this generation
    set(target_name "generate_${output_name}")
    add_custom_target(${target_name} DEPENDS ${ARG_OUTPUT})

    # Add dependency to the specified target (if provided)
    if(ARG_TARGET)
        if(TARGET ${ARG_TARGET})
            add_dependencies(${ARG_TARGET} ${target_name})
            target_sources(${ARG_TARGET} PUBLIC ${ARG_OUTPUT})
        else()
            message(WARNING "add_atlas_strong_types_inline: Target '${ARG_TARGET}' does not exist")
        endif()
    endif()
endfunction()

# Simplified helper for adding a single strong type with minimal syntax
#
# This is a convenience wrapper around add_atlas_strong_type() that reduces
# boilerplate for the most common use case.
#
# Parameters:
#   NAME - Required: Name of the strong type
#   TYPE - Required: Underlying C++ type
#   OPERATORS - Required: Atlas operators (e.g., "==, !=, <=>")
#   All other parameters from add_atlas_strong_type are forwarded
#
# Examples:
#   # Minimal usage
#   atlas_add_type(UserId int "==, !=, <=>")
#
#   # With additional options
#   atlas_add_type(Price double "+, -, *, /, <=>"
#       TARGET my_lib
#       DEFAULT_VALUE 0.0)
#
function(atlas_add_type NAME TYPE OPERATORS)
    # Auto-deduce namespace from directory structure
    _atlas_deduce_namespace(ns)

    # Forward to the full function with auto-deduced namespace
    add_atlas_strong_type(
        NAME ${NAME}
        TYPE ${TYPE}
        DESCRIPTION "${OPERATORS}"
        NAMESPACE ${ns} ${ARGN} # Forward any additional arguments
    )
endfunction()
