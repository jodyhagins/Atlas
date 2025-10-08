# CMake Integration

## The Easy Way: Helper Functions

Atlas provides convenient CMake helper functions that handle all the boilerplate for you. These are automatically available when you use `find_package(Atlas)` or `FetchContent`:

```cmake
include(FetchContent)
FetchContent_Declare(Atlas
    GIT_REPOSITORY https://github.com/jodyhagins/Atlas.git
    GIT_TAG main)
FetchContent_MakeAvailable(Atlas)

# Generate a single type with minimal syntax
atlas_add_type(UserId int "==, !=, <=>" TARGET my_library)
```

### Available Helper Functions

#### 1. `atlas_add_type()` - Quick single-type generation

```cmake
# Simplest form - namespace auto-deduced from directory structure
atlas_add_type(UserId int "==, !=, <=>")

# With target integration
atlas_add_type(Price double "+, -, *, /, <=>" TARGET my_lib)

# With additional options
atlas_add_type(Distance double "+, -, *, /, <=>"
    TARGET my_lib
    DEFAULT_VALUE 0.0
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/Distance.hpp)
```

#### 2. `add_atlas_strong_type()` - Full-featured generation

```cmake
add_atlas_strong_type(
    NAME Distance
    TYPE double
    DESCRIPTION "+, -, *, /, <=>"
    NAMESPACE geometry
    KIND struct
    DEFAULT_VALUE 0.0
    TARGET my_library)
```

#### 3. `add_atlas_strong_types_from_file()` - Generate from file

```cmake
add_atlas_strong_types_from_file(
    INPUT types.txt
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/Types.hpp
    TARGET my_library)
```

#### 4. `add_atlas_strong_types_inline()` - Inline definitions

```cmake
add_atlas_strong_types_inline(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/Types.hpp
    TARGET my_library
    CONTENT "
[type]
namespace=app
name=UserId
description=strong int; ==, !=, <=>

[type]
namespace=app
name=SessionId
description=strong int; ==, !=, hash
")
```

#### 5. `add_atlas_interactions_from_file()` - Cross-type interactions from file

```cmake
add_atlas_interactions_from_file(
    INPUT interactions.txt
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/Interactions.hpp
    TARGET my_library)
```

#### 6. `add_atlas_interactions_inline()` - Inline interaction definitions

```cmake
add_atlas_interactions_inline(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/CommerceOps.hpp
    TARGET my_library
    CONTENT "
include \"Price.hpp\"
include \"Quantity.hpp\"
include \"Total.hpp\"

namespace=commerce

Price * Quantity -> Total
Price * int <-> Price
Quantity * int <-> Quantity
")
```

See [`AtlasHelpers.md`](AtlasHelpers.md) for complete documentation of all helper functions, parameters, and examples.

## Advanced Integration: Manual Control

For cases where you need fine-grained control over the build process, you can use Atlas directly with CMake's `add_custom_command`:

### Using FetchContent with add_custom_command

```cmake
include(FetchContent)
FetchContent_Declare(Atlas
    GIT_REPOSITORY https://github.com/jodyhagins/Atlas.git
    GIT_TAG main)
FetchContent_MakeAvailable(Atlas)

# Escape semicolons because CMake treats them like list separators
set(DESC "strong int\\; ==, !=, <=>")

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/UserId.hpp
    COMMAND ${Atlas_EXECUTABLE}
        --kind=struct --namespace=myapp --name=UserId
        --description=${DESC}
        --output=${CMAKE_CURRENT_BINARY_DIR}/UserId.hpp
    DEPENDS Atlas::atlas
    VERBATIM)
```

### Creating a Reusable Function (DRY Principle)

If you need custom behavior beyond what the helper functions provide:

```cmake
function(generate_strong_type NAMESPACE TYPE_NAME DESCRIPTION)
    set(HEADER_FILE "${CMAKE_CURRENT_BINARY_DIR}/${TYPE_NAME}.hpp")
    string(REPLACE ";" "\\;" DESC_ESCAPED "${DESCRIPTION}")

    add_custom_command(
        OUTPUT ${HEADER_FILE}
        COMMAND ${Atlas_EXECUTABLE}
            --kind=struct
            --namespace=${NAMESPACE}
            --name=${TYPE_NAME}
            --description=${DESC_ESCAPED}
            --output=${HEADER_FILE}
        DEPENDS Atlas::atlas
        VERBATIM)

    set(GENERATED_HEADERS ${GENERATED_HEADERS} ${HEADER_FILE} PARENT_SCOPE)
endfunction()

# Now generate to your heart's content
generate_strong_type("myapp" "UserId" "strong int; ==, !=")
generate_strong_type("myapp" "Price" "strong double; +, -, *")
```

### Generating Interactions Manually

For cross-type interactions without helper functions:

```cmake
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/interactions.hpp
    COMMAND ${Atlas_EXECUTABLE}
        --input=${CMAKE_CURRENT_SOURCE_DIR}/interactions.txt
        --interactions=true
        --output=${CMAKE_CURRENT_BINARY_DIR}/interactions.hpp
    DEPENDS Atlas::atlas ${CMAKE_CURRENT_SOURCE_DIR}/interactions.txt
    VERBATIM)
```

**interactions.txt:**
```
include "Price.hpp"
include "Quantity.hpp"
include "Total.hpp"

namespace=commerce

Price * Quantity -> Total
Price * int <-> Price
```

### Using find_package (Traditional Installation)

For the control freaks who want to install things properly:

```bash
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
cmake --install build
```

Then in your project:
```cmake
find_package(Atlas REQUIRED)

add_custom_command(
    OUTPUT UserId.hpp
    COMMAND ${Atlas_EXECUTABLE} ...
    DEPENDS Atlas::atlas)
```

## Pro Tips

Speed up builds if you already have Boost installed:
```cmake
set(USE_SYSTEM_BOOST ON CACHE BOOL "Use system Boost")
FetchContent_MakeAvailable(Atlas)
```

## What You Get

| Variable/Target | Description |
|----------------|-------------|
| `Atlas::atlas` | The executable target (always use in DEPENDS) |
| `${Atlas_EXECUTABLE}` | Path to the binary (for COMMAND) |