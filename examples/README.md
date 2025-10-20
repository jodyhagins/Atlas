# Atlas Examples

Examples that demonstrate what this thing can actually do.

## CMake Integration Examples

New! Complete working examples showing how to integrate Atlas into CMake projects:

### **`fetchcontent_example/`** - Basic FetchContent integration
- Shows how to use Atlas with CMake's FetchContent
- Demonstrates the helper functions automatically available after `FetchContent_MakeAvailable()`
- Simple project setup with generated types

### **`helpers_basic_example/`** - CMake helper functions
- `atlas_add_type()` - Simplified single-type generation
- `add_atlas_strong_type()` - Full-featured type generation
- Target integration vs. manual file management
- Namespace auto-deduction

### **`helpers_inline_example/`** - Inline type definitions
- `add_atlas_strong_types_inline()` - Define types directly in CMakeLists.txt
- Multiple types in a single declaration
- No separate config files needed

### **`helpers_file_example/`** - File-based type definitions
- `add_atlas_strong_types_from_file()` - Generate from config file
- Best for projects with many types
- Domain-specific type organization

**Build any example:**
```bash
cd <example_directory>
cmake -B build -S .
cmake --build build
./build/<example_name>
```

**See also:** [cmake/AtlasHelpers.md](../cmake/AtlasHelpers.md) for complete CMake function reference

## Showcase Files

**`strong_types_showcase.txt`** - The greatest hits
- Multiple namespaces and base types (because one is never enough)
- All the operators: arithmetic, comparison, bitwise, stream
- Default values, hash support, subscripts
- Constexpr control for when compile-time evaluation betrays you
- **New features:** Optional `strong` keyword, global namespace support, named constants, inline type name syntax

**`interactions_showcase.txt`** - When types need to work together
- Cross-type operators (e.g., `Distance / Time -> Velocity`)
- Template interactions with C++20 concepts and C++17 SFINAE fallbacks
- Symmetric operations (`Price * int <-> Price`)
- Custom value accessors for the truly adventurous

## Generate Showcase Examples

Easy mode:
```bash
./generate_showcase.sh
```

Manual mode (for the DIY crowd):
```bash
cd ../build/atlas/debug && ninja

./bin/atlas --input=../../src/atlas/examples/strong_types_showcase.txt \
    --output=../../src/atlas/examples/generated/strong_types_showcase.hpp

./bin/atlas --interactions=true \
    --input=../../src/atlas/examples/interactions_showcase.txt \
    --output=../../src/atlas/examples/generated/interactions_showcase.hpp
```

Output lands in: `generated/strong_types_showcase.hpp`, `generated/interactions_showcase.hpp`