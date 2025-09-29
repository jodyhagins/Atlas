# CMake Integration Guide

This guide shows how to integrate Atlas into your CMake project using `FetchContent` or `find_package`.

---

## Option 1: FetchContent (Recommended for Easy Integration)

This is the easiest way to integrate Atlas into your CMake project. Atlas will be downloaded and built as part of your project's build process.

### Basic Setup

Add this to your `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.20)
project(MyProject)

include(FetchContent)

# Fetch Atlas
FetchContent_Declare(
    Atlas
    GIT_REPOSITORY https://github.com/jodyhagins/Atlas.git
    GIT_TAG main  # or specify a version tag like v1.0.0
)

FetchContent_MakeAvailable(Atlas)

# Now the atlas executable is available!
# Use it in your custom commands:
```

### Using Atlas to Generate Code

Once Atlas is fetched, you can use it in custom commands to generate headers during your build:

```cmake
# Define the output header location
set(GENERATED_HEADER "${CMAKE_CURRENT_BINARY_DIR}/generated/UserId.hpp")

# IMPORTANT: CMake treats semicolons as list separators.
# Escape semicolons in the description with backslash
set(USERID_DESC "strong int\\; ==, !=, <=>")

# Create a custom command that runs atlas
add_custom_command(
    OUTPUT ${GENERATED_HEADER}
    COMMAND ${Atlas_EXECUTABLE} --kind=struct --namespace=myapp::types --name=UserId --description=${USERID_DESC} > ${GENERATED_HEADER}
    DEPENDS Atlas::atlas
    COMMENT "Generating UserId strong type"
    VERBATIM
)

# Create a custom target that depends on the generated file
add_custom_target(generate_types
    DEPENDS ${GENERATED_HEADER}
)

# Add your main executable
add_executable(myapp main.cpp)

# Make sure the generated header is created before building
add_dependencies(myapp generate_types)

# Include the generated header directory
target_include_directories(myapp PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated)
```

### Complete Example

Here's a complete working example:

```cmake
cmake_minimum_required(VERSION 3.20)
project(MyStrongTypes LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Fetch Atlas
FetchContent_Declare(
    Atlas
    GIT_REPOSITORY https://github.com/jodyhagins/Atlas.git
    GIT_TAG main
)
FetchContent_MakeAvailable(Atlas)

# Directory for generated headers
set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY ${GENERATED_DIR})

# Function to generate a strong type
function(generate_strong_type NAMESPACE TYPE_NAME DESCRIPTION)
    set(HEADER_FILE "${GENERATED_DIR}/${TYPE_NAME}.hpp")

    # Escape semicolons in description for CMake
    string(REPLACE ";" "\\;" DESC_ESCAPED "${DESCRIPTION}")

    add_custom_command(
        OUTPUT ${HEADER_FILE}
        COMMAND ${Atlas_EXECUTABLE} --kind=struct --namespace=${NAMESPACE} --name=${TYPE_NAME} --description=${DESC_ESCAPED} > ${HEADER_FILE}
        DEPENDS Atlas::atlas
        COMMENT "Generating ${TYPE_NAME}"
        VERBATIM
    )

    # Add to the list of generated headers
    set(GENERATED_HEADERS ${GENERATED_HEADERS} ${HEADER_FILE} PARENT_SCOPE)
endfunction()

# Generate multiple types (semicolons are fine here since the function handles escaping)
generate_strong_type("myapp" "UserId" "strong int; ==, !=, <=>")
generate_strong_type("myapp" "Price" "strong double; +, -, *, /, ==, !=, <=>")
generate_strong_type("myapp" "EmailAddress" "strong std::string; ==, !=")

# Create target for all generated code
add_custom_target(generate_all_types
    DEPENDS ${GENERATED_HEADERS}
)

# Your application
add_executable(myapp main.cpp)
add_dependencies(myapp generate_all_types)
target_include_directories(myapp PRIVATE ${GENERATED_DIR})
```

---

## Option 2: Using Installed Atlas with find_package

If you've installed Atlas system-wide or to a specific location, you can use `find_package`:

```cmake
cmake_minimum_required(VERSION 3.20)
project(MyProject)

# Find the installed Atlas package
find_package(Atlas REQUIRED)

# The atlas executable is now available via ${Atlas_EXECUTABLE}
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/UserId.hpp
    COMMAND ${Atlas_EXECUTABLE}
        --kind=struct
        --namespace=myapp
        --name=UserId
        --description="strong int; ==, !=, <=>"
        > ${CMAKE_CURRENT_BINARY_DIR}/UserId.hpp
    DEPENDS Atlas::atlas
    COMMENT "Generating UserId"
)
```

To install Atlas to a custom location:

```bash
cd atlas
cmake -B build -DCMAKE_INSTALL_PREFIX=/path/to/install
cmake --build build
cmake --install build
```

Then configure your project with:

```bash
cmake -B build -DCMAKE_PREFIX_PATH=/path/to/install
```

---

## Option 3: Using System Boost (For Faster Builds)

If you have Boost installed on your system, you can tell Atlas to use it instead of fetching it:

```cmake
FetchContent_Declare(
    Atlas
    GIT_REPOSITORY https://github.com/jodyhagins/Atlas.git
    GIT_TAG main
)

# Set this option before making Atlas available
set(USE_SYSTEM_BOOST ON CACHE BOOL "Use system Boost")

FetchContent_MakeAvailable(Atlas)
```

---

## Variables and Targets Provided

After `FetchContent_MakeAvailable(Atlas)` or `find_package(Atlas)`, you get:

| Variable/Target | Description |
|----------------|-------------|
| `Atlas::atlas` | Imported executable target (use as dependency) |
| `${Atlas_EXECUTABLE}` | Full path to the atlas executable |
| `Atlas_VERSION` | Version of Atlas (find_package only) |

---

## Tips and Best Practices

### 1. **Use DEPENDS Atlas::atlas**

Always add `DEPENDS Atlas::atlas` to your custom commands. This ensures Atlas is built before your code generation runs:

```cmake
add_custom_command(
    OUTPUT generated.hpp
    COMMAND ${Atlas_EXECUTABLE} ...
    DEPENDS Atlas::atlas  # This is important!
)
```

### 2. **Generate Headers to Build Directory**

Generate headers into `${CMAKE_CURRENT_BINARY_DIR}` or a subdirectory, not your source tree:

```cmake
set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
```

### 3. **Use VERBATIM for Command Arguments**

Always use `VERBATIM` in `add_custom_command` to ensure proper escaping:

```cmake
add_custom_command(
    OUTPUT header.hpp
    COMMAND ${Atlas_EXECUTABLE} --description="strong int; +, -"
    VERBATIM  # Ensures proper handling of quotes and special chars
)
```

### 4. **Create Reusable Functions**

Wrap atlas invocations in CMake functions for cleaner code (see the complete example above).

### 5. **Speed Up Development Builds**

Use system Boost if available:

```bash
cmake -B build -DUSE_SYSTEM_BOOST=ON
```

---

## Troubleshooting

### "Atlas_EXECUTABLE not set"

Make sure you've called either `FetchContent_MakeAvailable(Atlas)` or `find_package(Atlas)` before using `${Atlas_EXECUTABLE}`.

### Generated headers not found

Ensure you've added the generated directory to your include path:

```cmake
target_include_directories(myapp PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated)
```

### Custom commands not running

Make sure your target depends on the generated files or custom target:

```cmake
add_dependencies(myapp generate_types)
```

### Boost takes too long to fetch

Use system Boost with `-DUSE_SYSTEM_BOOST=ON`, or pin to a cached build by setting `FETCHCONTENT_BASE_DIR` once and reusing it across projects.

---

## Advanced: Multiple Configuration Example

For projects that generate many types, consider organizing them:

```cmake
# types.cmake - Define all your types in one place
set(STRONG_TYPES
    "UserId:strong int; ==, !=, <=>"
    "ProductId:strong int; ==, !=, <=>"
    "Price:strong double; +, -, *, /, ==, !=, <=>"
    "Quantity:strong int; +, -, ==, !=, <=>"
)

foreach(TYPE_DEF IN LISTS STRONG_TYPES)
    string(REPLACE ":" ";" TYPE_PARTS "${TYPE_DEF}")
    list(GET TYPE_PARTS 0 TYPE_NAME)
    list(GET TYPE_PARTS 1 TYPE_DESC)

    generate_strong_type("myapp::types" "${TYPE_NAME}" "${TYPE_DESC}")
endforeach()
```

This allows you to maintain all type definitions in one place and generate them systematically.