# Atlas CMake Helper Functions

This document describes the CMake helper functions provided by Atlas for easy integration into CMake-based projects.

## Overview

Atlas provides four CMake functions to simplify strong type generation:

1. **`atlas_add_type()`** - Simplified single-type generation with minimal syntax
2. **`add_atlas_strong_type()`** - Full-featured single-type generation with all options
3. **`add_atlas_strong_types_from_file()`** - Generate multiple types from a configuration file
4. **`add_atlas_strong_types_inline()`** - Define multiple types inline in CMakeLists.txt

These functions are automatically available when you use `find_package(Atlas)` or `FetchContent` with Atlas.

## Getting Started

### Using FetchContent

```cmake
include(FetchContent)

FetchContent_Declare(
    Atlas
    GIT_REPOSITORY https://github.com/yourusername/atlas.git
    GIT_TAG main
)

FetchContent_MakeAvailable(Atlas)

# Helper functions are now automatically available!
atlas_add_type(UserId int "==, !=, <=>")
```

### Using find_package

```cmake
find_package(Atlas REQUIRED)

# Helper functions are now automatically available!
atlas_add_type(UserId int "==, !=, <=>")
```

## Function Reference

### 1. atlas_add_type()

**Quick single-type generation with minimal syntax.**

```cmake
atlas_add_type(NAME TYPE OPERATORS [additional options...])
```

**Parameters:**
- `NAME` (required) - Name of the strong type
- `TYPE` (required) - Underlying C++ type to wrap
- `OPERATORS` (required) - Atlas operators (e.g., "==, !=, <=>")
- Additional parameters from `add_atlas_strong_type()` can be passed

**Features:**
- Auto-deduces namespace from directory structure
- Minimal syntax for common use cases
- Forwards all additional parameters to `add_atlas_strong_type()`

**Examples:**

```cmake
# Simplest form - namespace auto-deduced
atlas_add_type(UserId int "==, !=, <=>")

# With target integration
atlas_add_type(Price double "+, -, *, /, <=>" TARGET my_lib)

# With additional options
atlas_add_type(Distance double "+, -, *, /, <=>"
    TARGET my_lib
    DEFAULT_VALUE 0.0
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/Distance.hpp
)
```

### 2. add_atlas_strong_type()

**Full-featured single-type generation with all configuration options.**

```cmake
add_atlas_strong_type(
    NAME <name>
    TYPE <type>
    DESCRIPTION <description>
    [NAMESPACE <namespace>]
    [KIND <struct|class>]
    [DEFAULT_VALUE <value>]
    [GUARD_PREFIX <prefix>]
    [GUARD_SEPARATOR <separator>]
    [UPCASE_GUARD <true|false>]
    [OUTPUT <path>]
    [TARGET <target>]
)
```

**Parameters:**
- `NAME` (required) - Name of the strong type
- `TYPE` (required) - Underlying C++ type to wrap
- `DESCRIPTION` (required) - Atlas operators/features (e.g., "==, !=, <=>")
- `NAMESPACE` (optional) - C++ namespace (defaults to auto-deduced from directory)
- `KIND` (optional) - 'struct' or 'class' (defaults to 'struct')
- `DEFAULT_VALUE` (optional) - Default value for default constructor
- `GUARD_PREFIX` (optional) - Custom prefix for header guards
- `GUARD_SEPARATOR` (optional) - Separator for header guard components
- `UPCASE_GUARD` (optional) - Use uppercase header guards (true/false)
- `OUTPUT` (optional) - Output file path (defaults to `${CMAKE_CURRENT_SOURCE_DIR}/${NAME}.hpp`)
- `TARGET` (optional) - Target to add dependency to (if not specified, no target integration)

**Examples:**

```cmake
# Minimal usage with auto-deduced namespace
add_atlas_strong_type(
    NAME UserId
    TYPE int
    DESCRIPTION "==, !=, <=>")

# Full configuration
add_atlas_strong_type(
    NAME Distance
    TYPE double
    DESCRIPTION "+, -, *, /, <=>"
    NAMESPACE geometry
    KIND struct
    DEFAULT_VALUE 0.0
    GUARD_PREFIX GEOMETRY
    UPCASE_GUARD true
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/Distance.hpp
    TARGET my_library)

# Generate without target integration (manual control)
add_atlas_strong_type(
    NAME Handle
    TYPE size_t
    DESCRIPTION "->, ==, !=, bool"
    NAMESPACE system
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/Handle.hpp
    # No TARGET - just generates the file
)
```

### 3. add_atlas_strong_types_from_file()

**Generate multiple types from a configuration file.**

```cmake
add_atlas_strong_types_from_file(
    INPUT <path>
    OUTPUT <path>
    [TARGET <target>]
)
```

**Parameters:**
- `INPUT` (required) - Input file containing type definitions
- `OUTPUT` (required) - Output file path for generated code
- `TARGET` (optional) - Target to add dependency to

**Input File Format:**

```ini
# Optional file-level configuration
guard_prefix=MY_TYPES
guard_separator=_
upcase_guard=true

# Type definitions
[type]
kind=struct
namespace=example
name=Distance
description=strong double; +, -, *, /, <=>
default_value=0.0

[type]
kind=struct
namespace=example
name=Handle
description=strong size_t; ->, ==, !=, bool
```

**Examples:**

```cmake
# Generate types from file
add_atlas_strong_types_from_file(
    INPUT types.txt
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/GeneratedTypes.hpp
    TARGET my_library)

# Using relative path (resolved to CMAKE_CURRENT_SOURCE_DIR)
add_atlas_strong_types_from_file(
    INPUT config/strong_types.txt
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/Types.hpp)
```

### 4. add_atlas_strong_types_inline()

**Define multiple types directly in CMakeLists.txt without a separate file.**

```cmake
add_atlas_strong_types_inline(
    OUTPUT <path>
    CONTENT <definitions>
    [TARGET <target>]
)
```

**Parameters:**
- `OUTPUT` (required) - Output file path for generated code
- `CONTENT` (required) - Type definitions in Atlas input file format
- `TARGET` (optional) - Target to add dependency to

**Examples:**

```cmake
add_atlas_strong_types_inline(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/CommonTypes.hpp
    TARGET my_library
    CONTENT "
# Common application types
guard_prefix=COMMON_TYPES

[type]
kind=struct
namespace=app
name=UserId
description=strong int; ==, !=, <=>
default_value=-1

[type]
kind=struct
namespace=app
name=SessionId
description=strong int; ==, !=, <=>

[type]
kind=struct
namespace=app
name=Distance
description=strong double; +, -, *, /, <=>
default_value=0.0
"
)
```

## Namespace Auto-Deduction

When `NAMESPACE` is not specified, it's automatically deduced from the source directory structure:

- `/path/to/project/src/foo/bar/CMakeLists.txt` → namespace `foo::bar`
- `/path/to/project/src/geometry/CMakeLists.txt` → namespace `geometry`
- `/path/to/project/src/CMakeLists.txt` → namespace matches directory name

If the source tree doesn't follow this pattern, the project name (lowercase) is used as the namespace.

## Target Integration

All functions support optional `TARGET` parameter:

- **With TARGET:** Generated files are automatically added to the target's sources and the generation becomes a dependency
- **Without TARGET:** Files are generated but not automatically integrated (for manual control)

```cmake
# Automatic integration
atlas_add_type(UserId int "==, !=, <=>" TARGET my_lib)

# Manual control
atlas_add_type(UserId int "==, !=, <=>")
target_sources(my_lib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/UserId.hpp)
```

## Atlas Operators Reference

Common operators supported in the `DESCRIPTION` parameter:

### Arithmetic Operators
- **Binary:** `+, -, *, /, %, &, |, ^, <<, >>`
- **Unary:** `u+, u-, u~`
- **Increment/Decrement:** `++, --`

### Comparison Operators
- **Traditional:** `==, !=, <, <=, >, >=`
- **Three-way:** `<=>`

### Special Operators
- **Conversion:** `bool` - boolean conversion
- **Function call:** `()` - call operator
- **Address-of:** `(&)` - address-of operator
- **Subscript:** `[]` - array subscript
- **At operator:** `@` - safe array access
- **Address-of value:** `&of` - address of underlying value
- **Member access:** `->` - pointer-like access

### Stream Operators
- `in` - input stream (`operator>>`)
- `out` - output stream (`operator<<`)

### Hash Support
- `hash` - enables `std::hash` specialization

### Constexpr Control
- `no-constexpr-hash` - disable constexpr hash (for runtime-only types)

See the [Atlas documentation](../README.md) for the complete operator list and advanced features.

## Output File Locations

By default, generated files are placed in the **source tree** (not the build tree) to ensure:
- Files are version controlled
- IDEs can navigate to them
- They're included in project distribution

```cmake
# Default: ${CMAKE_CURRENT_SOURCE_DIR}/${NAME}.hpp
atlas_add_type(UserId int "==, !=, <=>")
# Generates: ${CMAKE_CURRENT_SOURCE_DIR}/UserId.hpp

# Custom location
atlas_add_type(UserId int "==, !=, <=>"
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/include/types/UserId.hpp)
```

## Complete Examples

### Example 1: Simple Library

```cmake
cmake_minimum_required(VERSION 3.20)
project(MyProject LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(Atlas GIT_REPOSITORY ... GIT_TAG ...)
FetchContent_MakeAvailable(Atlas)

add_library(my_lib INTERFACE)

# Generate types with auto-deduced namespace
atlas_add_type(UserId int "==, !=, <=>" TARGET my_lib)
atlas_add_type(Price double "+, -, *, /, <=>" TARGET my_lib)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE my_lib)
```

### Example 2: File-based Types

```cmake
# Create types.txt with your type definitions
add_atlas_strong_types_from_file(
    INPUT types.txt
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/DomainTypes.hpp
    TARGET domain_lib)
```

**types.txt:**
```ini
guard_prefix=DOMAIN_TYPES

[type]
namespace=domain
name=CustomerId
description=strong int; ==, !=, <=>

[type]
namespace=domain
name=OrderId
description=strong int; ==, !=, <=>
```

### Example 3: Inline Types

```cmake
add_atlas_strong_types_inline(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/Types.hpp
    TARGET app_lib
    CONTENT "
[type]
namespace=app
name=SessionId
description=strong uint64_t; ==, !=, hash

[type]
namespace=app
name=Timestamp
description=strong int64_t; ==, !=, <, <=, >, >=, <=>, out
")
```

### Example 4: Mixed Approaches

```cmake
# Single types with specific needs
atlas_add_type(UserId int "==, !=, <=>" TARGET my_lib)
atlas_add_type(SessionId int "==, !=, hash" TARGET my_lib)

# Multiple related types from file
add_atlas_strong_types_from_file(
    INPUT domain_types.txt
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/DomainTypes.hpp
    TARGET my_lib)

# Quick inline definitions for local types
add_atlas_strong_types_inline(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/LocalTypes.hpp
    TARGET my_lib
    CONTENT "
[type]
namespace=local
name=TempId
description=strong int; ==, !=
")
```

## Best Practices

1. **Namespace Organization:** Use directory structure to organize namespaces automatically
2. **File Placement:** Keep generated files in source tree for version control
3. **Target Integration:** Use `TARGET` parameter for automatic dependency management
4. **Configuration Files:** Use file-based approach for projects with many types
5. **Inline Definitions:** Use inline approach for small, local type sets
6. **Version Control:** Commit generated files to track type evolution

## Troubleshooting

### Atlas executable not found

**Error:** `Atlas executable not found. Ensure Atlas is available via find_package or FetchContent.`

**Solution:** Ensure you've called `FetchContent_MakeAvailable(Atlas)` or `find_package(Atlas REQUIRED)` before using helper functions.

### Target does not exist

**Warning:** `Target 'my_target' does not exist`

**Solution:** Ensure the target is created with `add_library()` or `add_executable()` before using it in helper functions.

### Namespace deduction issues

**Issue:** Auto-deduced namespace is not what you expect

**Solution:** Explicitly specify `NAMESPACE` parameter:
```cmake
add_atlas_strong_type(
    NAME UserId
    TYPE int
    DESCRIPTION "==, !=, <=>"
    NAMESPACE my::custom::namespace
)
```

## See Also

- [Atlas README](../README.md) - Main project documentation
- [Atlas Tool Documentation](../docs/atlas-tool.md) - Command-line tool reference
- [Examples Directory](../examples/) - Complete working examples
- [Type System Guide](../docs/type-system.md) - Understanding strong types
