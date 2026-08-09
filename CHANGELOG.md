# Changelog

All notable changes to Atlas will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

**Versioning scope:** Version numbers apply to the generated code's API. A major version bump means regenerated code may require changes to user code that consumes the generated types.

## [Unreleased]

### Added

- **`atlas::unwrap_t<T>` and `atlas::undress_t<T>`** - type-level counterparts to the
  `atlas::unwrap` and `atlas::undress` function objects. `unwrap_t` names the type one
  layer down and is SFINAE-friendly for types with no layer to remove; `undress_t` names
  the root type and is the identity for non-atlas, non-enum types.
- **`atlas::AtlasOfC<T, U>`, `atlas::StringAtlasC<T>`, `atlas::ArithmeticAtlasC<T>`** -
  concepts that constrain on the root type, plus `atlas::is_atlas_of`,
  `atlas::is_string_atlas`, and `atlas::is_arithmetic_atlas` traits for pre-C++20
  standards. `ArithmeticAtlasC` excludes `bool`. The category concepts are spelled as
  conjunctions including `AtlasTypeC` so that they subsume it during overload resolution.

### Fixed

- **A `<=>` interaction now also generates `operator==`.** C++20 rewrites `<`, `<=`,
  `>`, and `>=` from `operator<=>`, but it synthesizes `==` only from a *defaulted*
  one -- and a cross-type interaction operator cannot be defaulted. `exit_code != 0`
  therefore failed to compile, forcing users to write a paired `==` interaction for
  every `<=>` interaction. Atlas now emits the `==` overload (from which `!=` is
  rewritten) unless an explicit `==` interaction already covers that pair, in which
  case the explicit one wins.
- **Generated floating-point comparisons no longer warn under `-Wfloat-equal`.** A
  warning raised inside a generated header cannot be fixed by the user, so the
  generator now wraps its `==`/`!=` comparisons -- per-type, defaulted, and
  interaction alike -- in a diagnostic push/ignore. The `bool` conversion operator is
  wrapped too, since GCC reads the cast as a comparison against zero.
- **Generated `++`/`--` no longer warn under `-Wunevaluated-expression`.** Their
  `noexcept` specifiers write `++std::declval<T&>()`, which clang flags as a side
  effect in an unevaluated context. The binary arithmetic and bitwise operators
  already suppressed this; the increment operators were missed.
- **Generated headers now include `<string>`.** The preamble's `is_string_atlas` and
  `StringAtlasC` name `std::string` unconditionally, but `<string>` was only included
  when a constraint needed it. libc++ pulls it in transitively so the omission was
  invisible under clang; under libstdc++ any generated header failed to compile
  pre-C++20 with "'string' is not a member of 'std'".

### Documentation

- Documented the previously undocumented `atlas::is_atlas_type` trait and
  `atlas::AtlasTypeC` concept, which have always shipped in generated headers. Projects
  were reinventing `atlas_value_type` detection because these were not discoverable.
- Documented that a `<=>` interaction yields all six comparisons, matching what the
  per-type `<=>` docs already promised.

## [1.0.0] - 2025-01-02

First formal release of Atlas Strong Type Generator.

### Breaking Changes

These changes may require updates to code that uses Atlas-generated types:

- **Renamed `atlas::value()` to `atlas::undress()`** - More descriptive (i.e., less general) name, and it does exactly what it says it does.
- **Input file extension changed from `.txt` to `.atlas`** - Update your CMake configurations and input files
- **Removed namespace versioning** - Simplified namespace structure

### Added

- **`atlas::unwrap()`** - Single-layer unwrapping for strong types and enums (returns immediate wrapped type)
- **`atlas::cast<T>()`** - Type-safe casting that drills until castable to target type
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
