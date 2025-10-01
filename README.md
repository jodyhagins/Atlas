# Atlas

[![CI](https://github.com/jodyhagins/Atlas/actions/workflows/ci.yml/badge.svg)](https://github.com/jodyhagins/Atlas/actions/workflows/ci.yml)
[![codecov](https://codecov.io/gh/jodyhagins/Atlas/branch/main/graph/badge.svg)](https://codecov.io/gh/jodyhagins/Atlas)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)

Atlas is a C++20 code generator that produces strongly typed wrapper classes and cross-type operator interactions from compact domain-specific descriptions. It ships with reusable libraries (`StrongTypeGenerator` and `InteractionGenerator`) and a CLI tool (`atlas`) that turn type descriptions into ready-to-use headers with configurable operators, member access, and header guards.

---

## Key Features

### Strong Type Generation
- Generate `struct` or `class` wrappers around any underlying C++ type.
- Opt-in support for arithmetic, comparison, logical, streaming, callable, and pointer-like operators.
- All operations are `constexpr` by default, with options to opt-out globally or selectively.
- Automatic header guard synthesis with SHA1-based uniqueness.
- Heuristics for required `<header>` includes and support for user-specified includes.

### Cross-Type Interaction Generation (NEW)
- Define operator interactions between different types (e.g., `Price * Quantity -> Total`).
- Support for symmetric (commutative) and asymmetric operators.
- Template-based interactions with C++20 concepts and C++17 SFINAE support.
- Customizable value extraction for flexible type designs.
- Self-contained generated files with embedded `atlas::value` implementation.

### Developer Experience
- Command-line interface with descriptive validation and built-in help.
- Library API that can be embedded into larger C++ build systems.
- Comprehensive error messages with line numbers for debugging.

---

## Project Layout

```
atlas/
├── .github/
│   └── workflows/
│       └── ci.yml                   # GitHub Actions CI configuration
│
├── cmake/
│   ├── AtlasConfig.cmake.in         # CMake package config template
│   └── StrongTypeGeneratorConfig.cmake.in
│
├── docs/
│   ├── cmake-integration.md         # CMake integration guide
│   ├── description-language.md      # DSL reference
│   └── generated-header-guards.md   # Header guard documentation
│
├── examples/
│   └── fetchcontent_example/
│       ├── CMakeLists.txt
│       ├── main.cpp
│       └── README.md
│
├── src/
│   ├── lib/
│   │   ├── AtlasCommandLine.hpp     # Command-line parser
│   │   ├── AtlasCommandLine.cpp
│   │   ├── AtlasUtilities.hpp       # Shared utilities (SHA1, header guards)
│   │   ├── AtlasUtilities.cpp
│   │   ├── StrongTypeGenerator.hpp  # Strong type generator API
│   │   ├── StrongTypeGenerator.cpp
│   │   ├── InteractionGenerator.hpp # Cross-type interaction generator API
│   │   └── InteractionGenerator.cpp
│   └── tools/
│       └── atlas.cpp                # CLI tool entry point
│
├── tests/
│   ├── CMakeLists.txt
│   ├── doctest.hpp                     # Testing framework header
│   ├── rapidcheck.hpp                  # Property-based testing header
│   ├── atlas_command_line_ut.cpp       # Command-line tests
│   ├── atlas_tool_ut.cpp               # Tool integration tests
│   ├── error_handling_ut.cpp           # Error handling tests
│   ├── generated_code_ut.cpp           # Generated code compilation tests
│   ├── integration_ut.cpp              # End-to-end integration tests
│   ├── interaction_generator_ut.cpp    # Interaction generator tests
│   └── strong_type_generator_ut.cpp    # Strong type generator tests
│
├── CHANGELOG.md
├── CMakeLists.txt
├── CODE_OF_CONDUCT.md
├── CONTRIBUTING.md
├── LICENSE
└── README.md
```

---

## Quick Start: Integrating Atlas Into Your CMake Project

The easiest way to use Atlas is via CMake's `FetchContent`:

```cmake
include(FetchContent)

FetchContent_Declare(
    Atlas
    GIT_REPOSITORY https://github.com/jodyhagins/Atlas.git
    GIT_TAG main
)
FetchContent_MakeAvailable(Atlas)

# Generate a strong type header (command-line mode)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/UserId.hpp
    COMMAND ${Atlas_EXECUTABLE}
        --kind=struct
        --namespace=myapp
        --name=UserId
        --description="strong int; ==, !=, <=>"
        --output=${CMAKE_CURRENT_BINARY_DIR}/UserId.hpp
    DEPENDS Atlas::atlas
)

# Or generate multiple types from an input file
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/strong_types.hpp
    COMMAND ${Atlas_EXECUTABLE}
        --input=${CMAKE_CURRENT_SOURCE_DIR}/types.txt
        --output=${CMAKE_CURRENT_BINARY_DIR}/strong_types.hpp
    DEPENDS Atlas::atlas ${CMAKE_CURRENT_SOURCE_DIR}/types.txt
)
```

**For complete integration examples and best practices, see [`docs/cmake-integration.md`](docs/cmake-integration.md).**

---

## Building Atlas Standalone

If you want to build and install Atlas itself:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cmake --install build --prefix /usr/local  # or your preferred location
```

### Building and Running Tests

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cd build && ctest --output-on-failure
```

Test discovery happens automatically at build time using DocTest's CMake integration.

### Build Prerequisites

- A C++20-capable compiler (e.g., GCC ≥ 11, Clang ≥ 15, MSVC ≥ 19.29)
- CMake ≥ 3.20
- Git (for fetching dependencies)
- Optional: System Boost installation (use `-DUSE_SYSTEM_BOOST=ON` to skip fetching Boost)
- Optional: Disable tests with `-DATLAS_BUILD_TESTS=OFF` (tests are built by default)

---

## Using the CLI

Atlas supports two modes of operation:

1. **Command-Line Mode**: Generate a single type by specifying parameters on the command line
2. **File Input Mode**: Generate multiple types from an input file specification

### Command-Line Mode (Single Type)

Use this mode to generate a single strong type directly from command-line arguments:

```bash
atlas --kind=<struct|class> \
      --namespace=<namespace> \
      --name=<type_name> \
      --description="strong <type>; <options...>" \
      [optional flags...]
```

| Flag               | Required | Description                                                                 |
|--------------------|----------|-----------------------------------------------------------------------------|
| `--kind`           | Yes*     | `struct` (public members) or `class` (private members with `public:` spec). |
| `--namespace`      | Yes*     | Namespace for the generated type (e.g., `app::types`).                      |
| `--name`           | Yes*     | Type name (may include scopes like `Outer::Inner::Name`).                   |
| `--description`    | Yes*     | DSL description of the wrapped type and operator options.                   |
| `--guard-prefix`   | No       | Override the default header guard prefix.                                   |
| `--guard-separator`| No       | Separator for header guard tokens (default `_`).                            |
| `--upcase-guard`   | No       | `true`/`false` toggling uppercase header guards (default `true`).           |
| `--output`         | No       | Write generated code to a file instead of stdout.                           |
| `--help`, `-h`     | No       | Print help text and exit.                                                   |

\* Required in command-line mode (not required if using `--input`)

#### Example

```bash
atlas \
  --kind=struct \
  --namespace=geom \
  --name=Length \
  --description="strong double; +, -, ==, !=, <=>, out, bool"
```

Generates a header that wraps `double` while enabling arithmetic, comparison, streaming, and boolean conversion operators.

### File Input Mode (Multiple Types)

Use this mode to generate multiple strong types from an input file. All types will be placed in a single header file with a unified header guard:

```bash
atlas --input=<file> [--output=<file>] [optional flags...]
```

| Flag               | Required | Description                                                                 |
|--------------------|----------|-----------------------------------------------------------------------------|
| `--input`          | Yes      | Input file containing type definitions.                                     |
| `--output`         | No       | Write generated code to a file instead of stdout.                           |
| `--guard-prefix`   | No       | Override the header guard prefix (rarely needed in file mode).              |
| `--guard-separator`| No       | Can be overridden, but usually set in the input file.                       |
| `--upcase-guard`   | No       | Can be overridden, but usually set in the input file.                       |

#### Input File Format

The input file uses a simple key=value format with `[type]` section markers:

```txt
# File-level configuration (optional)
guard_prefix=<PREFIX>           # optional prefix for header guard
guard_separator=_               # optional, default: _
upcase_guard=true               # optional, default: true

# Type definitions
[type]
kind=<struct|class>
namespace=<namespace>
name=<TypeName>
description=<strong type description>

[type]
kind=<struct|class>
namespace=<namespace>
name=<AnotherType>
description=<strong type description>
```

- Lines starting with `#` are comments
- Empty lines are ignored
- `guard_prefix` is optional - if provided, the guard will be `PREFIX_SEPARATOR_SHA1`
- If no prefix is given, defaults to `ATLAS_SEPARATOR_SHA1` (ensures valid C++ identifier)
- Each `[type]` section defines one strong type
- **All types share a single header guard** with SHA1 computed from combined content

#### Example Input File (`types.txt`)

```txt
# Strong types for my application
guard_prefix=MY_APP_TYPES

[type]
kind=struct
namespace=math
name=Distance
description=strong int; +, -, ==, !=, <=>

[type]
kind=class
namespace=util
name=Counter
description=strong int; ++, --, bool, out

[type]
kind=struct
namespace=geo
name=Point
description=strong double; +, -, *, /, <=>
```

#### Usage

```bash
# Generate to stdout
atlas --input=types.txt

# Generate to file
atlas --input=types.txt --output=strong_types.hpp
```

This generates all three type definitions in a single output file with one unified header guard (e.g., `MY_APP_TYPES_A1B2C3...`).

---

## Cross-Type Interaction Mode

Atlas can generate operator interactions between different types, useful for scenarios like units of measure (`Distance = Velocity * Time`) or domain-specific calculations (`Total = Price * Quantity`).

### Interactions Mode

```bash
atlas --input=<file> --interactions=true [--output=<file>]
```

| Flag              | Required | Description                                          |
|-------------------|----------|------------------------------------------------------|
| `--input`         | Yes      | Input file containing interaction definitions.       |
| `--interactions`  | Yes      | Must be `true` to enable interactions mode.          |
| `--output`        | No       | Write generated code to a file instead of stdout.    |

### Interaction File Format

```txt
# Include necessary headers
include <iostream>
include "price.hpp"
include "quantity.hpp"

# Set namespace for interactions
namespace=commerce

# Asymmetric operator: Price * Quantity -> Total
Price * Quantity -> Total

# Symmetric operator (commutative): Price * int <-> Price
# Generates both: Price * int -> Price AND int * Price -> Price
Price * int <-> Price
Quantity * int <-> Quantity

# Change namespace
namespace=math

# Template interactions with concepts (C++20) and SFINAE (C++17)
concept=Numeric
enable_if=std::is_arithmetic_v<Numeric>

Distance * Numeric <-> Distance
Distance / Numeric -> Distance

# Control constexpr
no-constexpr
Time / Numeric -> Time
```

### Interaction Directives

| Directive            | Description                                                      |
|----------------------|------------------------------------------------------------------|
| `include <header>`   | Add system header include.                                       |
| `include "header"`   | Add local header include.                                        |
| `namespace=name`     | Set namespace for subsequent interactions.                       |
| `concept=Name`       | Define a template parameter as a C++20 concept.                  |
| `enable_if=expr`     | Add C++17 SFINAE constraint (used with concept).                 |
| `constexpr`          | Make subsequent operators constexpr (default).                   |
| `no-constexpr`       | Make subsequent operators non-constexpr.                         |
| `value_access=func`  | Customize value extraction (default: `atlas::value`).            |

### Interaction Syntax

```txt
LHS operator RHS -> Result    # Asymmetric: only generates LHS op RHS
LHS operator RHS <-> Result   # Symmetric: generates both LHS op RHS and RHS op LHS
```

Supported operators: `+`, `-`, `*`, `/`, `%`, `&`, `|`, `^`, `<<`, `>>`, `==`, `!=`, `<`, `>`, `<=`, `>=`, `&&`, `||`

### Example: Units of Measure

```txt
# physics_interactions.txt
include <type_traits>
include "distance.hpp"
include "velocity.hpp"
include "time.hpp"

namespace=physics

concept=Numeric
enable_if=std::is_arithmetic_v<Numeric>

# Distance = Velocity * Time
Velocity * Time -> Distance

# Velocity = Distance / Time
Distance / Time -> Velocity

# Scalar operations
Distance * Numeric <-> Distance
Distance / Numeric -> Distance
Velocity * Numeric <-> Velocity
```

Generate with:
```bash
atlas --input=physics_interactions.txt --interactions=true --output=physics_ops.hpp
```

This creates a self-contained header with:
- All specified includes
- Embedded `atlas::value` implementation for value extraction
- Operator functions in the specified namespace
- Template specializations with C++20 concepts (when available) and C++17 SFINAE fallback

---

## Description Language

Type descriptions follow the pattern:

```txt
strong <underlying-type>; <option>, <option>, ...
```

Options enable operator overloads, pointer-like behavior, callables, header includes, and more. See `docs/description-language.md` for a full reference covering:

- Arithmetic (`+`, `-*`, etc.)
- Logical (`&&`, `||`, `not`)
- Comparison (`==`, `<=>`, etc.)
- Stream (`in`, `out`)
- Callable and pointer-like semantics (`()`, `(&)`, `@`, `&of`, `->`)
- Explicit includes (`#<header>` or `#"path"`)

---

## Generated Code Overview

Headers created by Atlas include:

- Automatic include directives (deduced from the DSL and heuristics).
- Explicit constructor templates guarding constructibility.
- Explicit conversion operators to the underlying type (and `bool` when requested).
- Friend operators defined based on selected options.
- Header guards structured as `PREFIX_SEPARATOR_SHA1` (details in `docs/generated-header-guards.md`).

---

## Library API

### Strong Type Generation

`StrongTypeGenerator` exposes a callable object:

```cpp
#include "StrongTypeGenerator.hpp"

wjh::atlas::StrongTypeDescription desc{
    .kind = "struct",
    .type_namespace = "geom",
    .type_name = "Length",
    .description = "strong double; +, -",
    .guard_prefix = "",
    .guard_separator = "_",
    .upcase_guard = true
};

std::string header = wjh::atlas::generate_strong_type(desc);
```

### Interaction Generation

`InteractionGenerator` exposes a callable object:

```cpp
#include "InteractionGenerator.hpp"

wjh::atlas::InteractionFileDescription desc;
desc.includes = {"<iostream>", "\"price.hpp\""};
desc.interactions.push_back(wjh::atlas::InteractionDescription{
    .op_symbol = "*",
    .lhs_type = "Price",
    .rhs_type = "Quantity",
    .result_type = "Total",
    .symmetric = false,
    .lhs_is_template = false,
    .rhs_is_template = false,
    .is_constexpr = true,
    .interaction_namespace = "commerce",
    .value_access = "atlas::value"
});

std::string header = wjh::atlas::generate_interactions(desc);
```

Use the returned strings as standalone headers or embed Atlas in your own generators.

---

## Development

- Code style follows standard C++20 practices.
- Proper code formatting requires a fork of clang-format.
- New features should include documentation updates and entries in `CHANGELOG.md`.
- Contributions require appropriate tests.

---

## Contributing

We welcome issues, feature requests, and pull requests. Please read `CONTRIBUTING.md` for workflow details and `CODE_OF_CONDUCT.md` for community expectations.

---

## AI-Assisted Development

Atlas serves a dual purpose: it's both a practical tool for C++20 strong type generation and an experimental playground for AI-assisted software development.

### Development Approach

The core strong type generation logic was written manually to establish the foundation. However, **almost all subsequent development was AI-generated**, including:

- **Documentation**: README, contribution guidelines, and API documentation
- **Test suites**: Unit tests, integration tests, and property-based tests
- **Features**: File input/output, command-line parsing, and configuration options
- **Refactoring**: Code organization, separation of concerns, and library structure

This project demonstrates that AI can be effective at:
- Implementing well-specified features with clear requirements
- Writing comprehensive test coverage
- Generating consistent, well-structured documentation
- Refactoring code according to design principles

The human developer provides architectural decisions, design constraints, and quality feedback, while the AI handles the implementation details.

### Implications for Software Engineering

Atlas serves as a case study in AI-assisted development workflows. The codebase shows that with proper human guidance and feedback loops, AI can contribute meaningfully to production-quality software development, from initial implementation through refactoring and documentation. Time will tell whether this is true.

---

## License

Atlas is available under the MIT License. See `LICENSE` for full text.

