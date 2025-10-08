# Atlas

[![CI](https://github.com/jodyhagins/Atlas/actions/workflows/ci.yml/badge.svg)](https://github.com/jodyhagins/Atlas/actions/workflows/ci.yml)
[![codecov](https://codecov.io/gh/jodyhagins/Atlas/branch/main/graph/badge.svg)](https://codecov.io/gh/jodyhagins/Atlas)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)

> *"I got tired of writing the same boilerplate strong type wrappers over and over, so I wrote a tool to generate them. Then I spent 10x more time perfecting the tool than I would've spent just writing the types. This is the way."*

A C++20 code generator that turns terse descriptions into strongly typed wrappers.
Because `execute(int64_t bid_price, int64_t bid_qty, int64_t ask_price, int64_t ask_qty)`
is a serious bug and changing it to
`execute(Price bid_price, Quantity bid_qty, Price ask_price, Quantity ask_qty)`
buys you just enough time to have a production issue in time to tank next quarter's bonus too.

## Quick Start

The easiest way to use Atlas in your CMake project:

```cmake
include(FetchContent)
FetchContent_Declare(Atlas
    GIT_REPOSITORY https://github.com/jodyhagins/Atlas.git
    GIT_TAG main)
FetchContent_MakeAvailable(Atlas)

# Generate a strong type with minimal syntax
atlas_add_type(UserId int "==, !=, <=>" TARGET my_library)
```

That's it! The helper function automatically:
- Generates the header file in your source directory
- Adds it to your target's sources
- Sets up proper build dependencies

### More Helper Functions

```cmake
# Generate multiple types from a file
add_atlas_strong_types_from_file(
    INPUT types.txt
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/Types.hpp
    TARGET my_library)

# Generate cross-type interactions (e.g., Price * Quantity -> Total)
add_atlas_interactions_inline(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/Interactions.hpp
    TARGET my_library
    CONTENT "
include \"Price.hpp\"
include \"Quantity.hpp\"
include \"Total.hpp\"

namespace=commerce

Price * Quantity -> Total
Price * int <-> Price
")
```

See [`docs/AtlasHelpers.md`](docs/AtlasHelpers.md) for complete reference of all helper functions, or [`docs/cmake-integration.md`](docs/cmake-integration.md) for advanced integration patterns.

## Installation

Build and install:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cmake --install build --prefix /usr/local
```

Run tests:
```bash
cd build && ctest --output-on-failure
```

Run code coverage:
```bash
cmake --build build/coverage --target coverage
# See docs/COVERAGE.md for details
```

Requirements: C++20 compiler (GCC ≥11, Clang ≥15), CMake ≥3.20, and the crushing realization that strong types actually matter

## Usage

### Strong Types

Generate a single type:
```bash
atlas --kind=struct --namespace=geom --name=Length \
      --description="strong double; +, -, ==, !=, <=>" \
      --output=Length.hpp
```

Generate multiple types from a file (batch processing your type safety):
```bash
atlas --input=types.txt --output=strong_types.hpp
```

**types.txt:**
```
[type]
kind=struct
namespace=math
name=Distance
description=strong int; +, -, ==, <=>

[type]
kind=struct
namespace=util
name=Counter
description=strong int; ++, --, out
```

### Cross-Type Interactions

Make types play nicely with each other (like all humans do):
```bash
atlas --input=interactions.txt --interactions=true --output=ops.hpp
```

**interactions.txt:**
```
include "price.hpp"
include "quantity.hpp"

namespace=commerce

Price * Quantity -> Total
Price * int <-> Price        # Symmetric (because it's is commutative)

namespace=physics
concept=Numeric
enable_if=std::is_arithmetic_v<Numeric>

Distance * Numeric <-> Distance
Distance / Time -> Velocity
```

## Description Language

Syntax: `strong <type>; <options...>`

Common options (because we're all about that opt-in life):
- Arithmetic: `+`, `-`, `*`, `/`, `++`, `--` — when your type needs to do math
- Comparison: `==`, `!=`, `<=>`, `<`, `>` — for when sorting matters
- Stream: `in`, `out` — `std::cout` compatibility sold separately
- Callable: `()`, `(&)` — yes, your strong type can be a functor. Why? Because C++.
- Pointer-like: `@`, `->`, `&of` — make it look like a pointer without the segfaults
- Containers: `[]`, `hash`, `iterable` — for those fancy data structures
- Special: `bool`, `no-constexpr` — the "it's complicated" options

Full reference (for the masochists): [`docs/description-language.md`](docs/description-language.md)

## Library API

Embed Atlas in your own build tools:

```cpp
#include "StrongTypeGenerator.hpp"

wjh::atlas::StrongTypeDescription desc{
    .kind = "struct",
    .type_namespace = "geom",
    .type_name = "Length",
    .description = "strong double; +, -"
};
std::string header = wjh::atlas::generate_strong_type(desc);
```

```cpp
#include "InteractionGenerator.hpp"

wjh::atlas::InteractionFileDescription desc;
desc.includes = {"\"price.hpp\""};
desc.interactions.push_back({
    .op_symbol = "*",
    .lhs_type = "Price",
    .rhs_type = "Quantity",
    .result_type = "Total",
    .interaction_namespace = "commerce"
});
std::string header = wjh::atlas::generate_interactions(desc);
```

## Documentation

- **[Description Language Reference](docs/description-language.md)** - Complete syntax guide
- **[CMake Integration](docs/cmake-integration.md)** - Usage in CMake projects
- **[Code Coverage](docs/COVERAGE.md)** - Coverage analysis guide
- **[Header Guards](docs/generated-header-guards.md)** - Customizing generated guards

## Contributing

Contributions welcome! See [`docs/CONTRIBUTING.md`](docs/CONTRIBUTING.md) for guidelines and [`docs/CODE_OF_CONDUCT.md`](docs/CODE_OF_CONDUCT.md) to learn how to be a decent human while coding.

## License

MIT License. See [`LICENSE`](LICENSE) for the legal stuff. TL;DR: Do whatever you want, just don't blame me.
