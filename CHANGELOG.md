# Changelog

All notable changes to Atlas will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

**Versioning scope:** Version numbers apply to the generated code's API. A major version bump means regenerated code may require changes to user code that consumes the generated types.

## [Unreleased]

## [1.0.0] - 2025-01-02

First formal release of Atlas Strong Type Generator.

### Breaking Changes

These changes may require updates to code that uses Atlas-generated types:

- **Renamed `atlas::value()` to `atlas::undress()`** - More descriptive (i.e., less general) name, and it does exactly what it says it does.
- **Input file extension changed from `.txt` to `.atlas`** - Update your CMake configurations and input files
- **Removed namespace versioning** - Simplified namespace structure

### Added

- **`atlas::cast<T>()`** - Type-safe casting between strong types
- **Drill-down support for hash/format/istream/ostream** - Nested type support for standard library integration
- **User-defined strong type templates** - Custom template definitions for advanced use cases
- **Spaceship operator (`<=>`) in interactions** - Three-way comparison support for cross-type operations
- **C++17 fallback for spaceship operator** - Automatic generation of comparison operators when `<=>` not available
- **Optional type support** - Invalid value semantics with `nilable` types
- **Constrained types** - Validation constraints including:
  - `positive` - Values must be > 0
  - `non_negative` - Values must be >= 0
  - `bounded<min, max>` - Values within specified range
  - `non_empty` - Container types must not be empty
  - `non_null` - Pointer types must not be null
- **Arithmetic modes** - `wrapping`, `saturating`, and `checked` arithmetic
- **Member function forwarding** - Forward underlying type methods with return type transformation
- **C++ standard specification** - Target specific C++ standards (11, 14, 17, 20, 23) with compile-time enforcement
- **Automatic include detection** - Type tokenization for automatic header inclusion
- **SFINAE-based operator forwarding** - Smart `operator->` and `operator*` forwarding for pointer-like types
- **Named constants** - Static member constants with `constants=name:value; name2:value2` syntax
- **Inline type name syntax** - `[struct namespace::TypeName]` shorthand in file mode
- **Global namespace support** - Use `namespace=` (empty) for types in the global namespace
- **CMake interaction helpers** - `add_atlas_interactions_from_file()` and `add_atlas_interactions_inline()`
- **Iterator support** - `iterable` feature for range-based for loops

### Changed

- BasicNilable moved-from objects now set to nil_value
- Refactored StrongTypeGenerator for improved maintainability
- Made `strong` keyword optional in type descriptions
- Made `struct` the default kind when omitted in file mode
- Unified code generation with consistent include handling
- Made `operator->` and `operator*` more consistent

### Fixed

- Fixed `atlas::value()` implementation
- Fixed include duplication and description sorting inconsistency
- Ignore weak vtable warnings in generated code

[Unreleased]: https://github.com/jodyhagins/Atlas/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/jodyhagins/Atlas/releases/tag/v1.0.0
