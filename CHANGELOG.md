# Changelog

All notable changes to Atlas will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

**Versioning scope:** Version numbers apply to the generated code's API. A major version bump means regenerated code may require changes to user code that consumes the generated types.

## [2.0.0] - Unreleased

### Changed (BREAKING)

Version 2.0.0 closes two classes of lifetime and type-safety defects in
generated code. Both were reproduced with ASan against 1.x output.

- **Borrowing conversions from temporaries are now compile errors.** When the
  underlying type owns storage and a conversion target borrows it
  (`std::string` -> `std::string_view`), converting an rvalue dangled on every
  route. All five routes are now rejected at compile time:
  - `static_cast<std::string_view>(make_id())` and
    `static_cast<std::string_view>(std::move(id))`
  - copy-initialization / implicit conversion: `std::string_view sv = make_id();`
  - drilling: `atlas::cast<std::string_view>(make_id())` — including for types
    that declare no conversion clause at all
  - reference targets: `atlas::cast<T const &>(rvalue)` / `atlas::cast<T &&>(rvalue)`
  - the `from_atlas_cast` constructor of a view-backed type (which *stores*
    the borrow)

  Generated `cast<T>` / `explicit_cast<T>` / `implicit_cast<T>` conversions
  are now `const &`-qualified, and every type carries one deleted,
  non-explicit, rvalue-only conversion guard constrained on
  `atlas::borrow_policy`. Detection idioms (`std::is_constructible`,
  `requires`, `atlas::atlas_detail::is_directly_castable`) report `false`
  for guarded operations instead of reporting viable and then erroring.

  **Migration:** no `.atlas` edits needed. At a rejected call site, bind the
  temporary to a named variable and borrow from that. The only safe pattern
  this rejects is a borrow consumed within the same full expression
  (`f(static_cast<std::string_view>(make_x()))`) — bind a variable there too.

- **Reference-target casts require lvalue sources and bind only to actual
  layers.** `atlas::cast<T &>` / `<T const &>` / `<T &&>` from an rvalue
  returned a reference into a dying temporary; from an lvalue whose chain
  merely *converts* to `T`, they returned a reference to a materialized
  temporary. Both are now rejected; a reference target binds only to a real
  layer of an lvalue chain. To move out of a temporary, use a value target
  (`atlas::cast<int>(std::move(x))`), which drills by moving layer by layer.

- **`from_atlas_cast` now requires an explicit allowlist.** The bare token
  keyed only on the underlying type, so *every* type sharing a representation
  was mutually constructible (`SpawnTimeoutMs{TotallyUnrelatedCount{5}}`
  compiled). Now:
  - `from_atlas_cast<A, B, ...>` names the permitted source types; an
    argument is accepted only when it *is*, or *drills to*, a named type
    (an actual layer of its chain — sharing the underlying does not qualify).
  - bare `from_atlas_cast` is a generation-time error naming the type, its
    line, and both fixes.
  - a malformed list — unbalanced angle brackets (`from_atlas_cast<A>>`,
    `from_atlas_cast<A<B,C>`) or text after the closing `>` — is a
    generation-time error naming the token, instead of baking the imbalance
    into the emitted constraint.
  - `auto_from_atlas_cast=true` is narrowed to the types declared in the
    same file, and unions with explicit lists.

  **Migration:** replace `from_atlas_cast` with `from_atlas_cast<A, B>`
  listing the related types (one entry per permitted source), or set
  `auto_from_atlas_cast=true` for a family declared in one file.

### Added

- **Borrow classification traits** in the shared preamble, identical at every
  supported standard: `atlas::borrowed_type<T>` (C++11-compatible opt-in,
  seeded for `std::basic_string_view` and `std::span`), `atlas::is_borrowed<T>`
  (also honors `std::ranges::enable_borrowed_range` where available), and
  `atlas::borrow_policy<Target, Underlying>` (the predicate every guard
  checks; specializable per pair in either direction). `is_borrowed`
  classifies an Atlas-generated type by what it wraps — a wrapper is borrowed
  when any layer beneath it is — so strong types layered on strong types keep
  working: view-rooted chains convert, drill, and allowlist-construct from
  temporaries at every depth, owner-rooted chains stay guarded at every
  depth, and drilling an expiring owner *into* a view-rooted wrapper is
  guarded too. The recursion is limited to generated types (those deriving
  from `atlas::strong_type_tag`); a hand-written type adopting the
  `atlas_value_type` protocol may own storage while computing a view from
  it, so such types are never auto-classified and keep the explicit
  `atlas::borrowed_type` / `enable_borrowed_range` opt-in.
- **`borrowed_types=` file-level directive** declaring view types, third-party
  or defined in the same file (same-file names are forward-declared
  automatically ahead of the specializations). Each name gets an individually
  include-guarded `atlas::borrowed_type` specialization, so duplicate
  declarations across generated headers are benign; a value with unbalanced
  angle brackets or an empty name is a generation-time error. Hand-written
  specializations must be visible before any generated header in every
  translation unit (inconsistent visibility is IFNDR), and must not be
  combined with a `borrowed_types=` declaration for the same type in one
  translation unit.
- **Preamble version check.** The shared preamble defines
  `WJH_ATLAS_PREAMBLE_VERSION` inside its include guard, and every generated
  header verifies it outside that guard. Including a header generated by an
  older, incompatible Atlas *before* a newer one now fails with one clear
  `#error` naming the remedy (regenerate with one Atlas version) instead of
  confusing errors inside generated classes. The reverse order is inherently
  undetectable — a header generated before this check existed carries no
  check text — but is harmless while preamble revisions stay additive; the
  remedy either way is to regenerate everything with one Atlas version.
- **Deliberately unguarded:** `forward=` accessors and `iterable`'s
  `begin()`/`end()` keep the underlying type's value-category behavior, so
  `std::printf("%s", make_id().c_str())` and range-`for` over a temporary
  keep working. Atlas guards what it invents; it stays faithful where you
  ask for the underlying's own member.
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

- **Interaction headers now compose across modules.** Every generated interaction
  operator is wrapped in its own include guard, so two modules that each declare
  `Meters + Meters` can both be included in one translation unit instead of colliding.
  Previously only the compound-assignment helpers and `atlas_value_for` accessors were
  guarded, which left the binary operators themselves requiring a hand-enforced
  "declare it in exactly one module" convention. The guard hashes the complete
  definition, not just the signature, so two headers that declare the same signature
  with *different* bodies still collide and get diagnosed rather than silently
  resolving to whichever header was included first.
- **A symmetric interaction between identical operand types no longer emits the
  operator twice.** `Meters + Meters <-> Meters` generated the same
  `operator+(Meters, Meters)` for each direction, and the resulting header did not
  compile.
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
