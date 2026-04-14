# Design Problem: `no-constexpr-hash` and `auto_hash` Preamble

## Context

We just deprecated per-type `hash`, `out`, `in`, `fmt` tokens in favor of file-level `auto_*` directives (`auto_hash=true`, `auto_ostream=true`, etc.). The per-type tokens were misleading because they silently enabled file-wide behavior, not per-type code generation.

**Commit state:** All changes are in the working tree (not committed). ClassInfo.cpp has deprecation warnings, examples/tests/docs are updated, all 66 tests pass.

## The Problem

The `no-constexpr-hash` token was a per-type escape hatch:

```
[struct concurrency::ThreadId]
description=int; ==, !=, hash, no-constexpr-hash
```

It set `hash_const_expr = ""` on that specific type's `ClassInfo`, making that type's hash specialization non-constexpr. This was needed because some underlying types (like `std::string`) have `std::hash` implementations that cannot be `constexpr`.

With `auto_hash=true`, a **single generic preamble** is generated for the entire file -- a concept-constrained partial specialization of `std::hash<T>` that applies to all atlas types. There is no per-type code generation, so there's no place to apply per-type constexpr control.

**If the preamble is `constexpr`, it breaks for types with non-constexpr hash.**
**If the preamble is non-`constexpr`, types that could be constexpr lose that property.**

## What the Auto Hash Preamble Looks Like Today

The preamble (generated once per file) is roughly:

```cpp
template <typename T>
    requires atlas::is_atlas_type<T>
struct std::hash<T>
{
    constexpr std::size_t operator()(T const & v) const noexcept
    {
        return std::hash<std::remove_cvref_t<decltype(atlas::undress(v))>>{}(
            atlas::undress(v));
    }
};
```

The `constexpr` on `operator()` is the problem. It works for `int`, `unsigned long`, etc. but fails for `std::string` because `std::hash<std::string>::operator()` is not `constexpr`.

## Who Uses `no-constexpr-hash`?

In the atlas codebase:
- `examples/strong_types_showcase.atlas`: Had it on `UserId` (removed during deprecation migration) and `ThreadId` (removed during migration)
- `tests/constraint_integration_test_types.atlas`: Had it on `BoundedWithHash` and `ComplexComposition` (removed -- file has `auto_hash=true`)
- `tests/constraint_integration2_test_types.atlas`: Same
- Golden test `.input` files: Still use it (unchanged per plan)

In practice, any type wrapping `std::string`, `std::vector`, or other non-trivially-hashable types needs non-constexpr hash.

## Candidate Solutions

### Option 1: Always non-constexpr

Make the auto hash preamble always non-`constexpr`:

```cpp
std::size_t operator()(T const & v) const noexcept { ... }
```

**Pros:** Simple, always correct, no special cases.
**Cons:** Loses `constexpr` hash for types that could support it (integral types, etc.). In practice, `constexpr` hash is rarely needed at compile time.

### Option 2: SFINAE/concept detection

Try to detect at compile time whether `std::hash<underlying>` is constexpr-invocable:

```cpp
// Hypothetical
template <typename T>
concept constexpr_hashable = requires(T v) {
    { std::hash<T>{}(v) } -> std::same_as<std::size_t>;
    // But there's no way to test "is this constexpr" in a concept
};
```

**Problem:** C++ has no mechanism to test "is this expression a constant expression" in SFINAE or concepts. `std::is_constant_evaluated()` only works inside a constexpr function at runtime, not in template constraints. This approach is fundamentally blocked by the language.

### Option 3: Opt-in type trait

Define a trait that identifies types with constexpr-capable hash:

```cpp
namespace atlas {
template <typename T>
struct is_constexpr_hashable : std::false_type {};

// Specialize for known-constexpr types
template <> struct is_constexpr_hashable<int> : std::true_type {};
template <> struct is_constexpr_hashable<unsigned> : std::true_type {};
template <> struct is_constexpr_hashable<long> : std::true_type {};
// ... all integral types, enum types, etc.
}
```

Then the preamble generates two specializations:

```cpp
// Constexpr version for types with constexpr hash
template <typename T>
    requires atlas::is_atlas_type<T> &&
             atlas::is_constexpr_hashable<decltype(atlas::undress(std::declval<T>()))>
struct std::hash<T>
{
    constexpr std::size_t operator()(T const & v) const noexcept { ... }
};

// Non-constexpr version for everything else
template <typename T>
    requires atlas::is_atlas_type<T> &&
             (!atlas::is_constexpr_hashable<decltype(atlas::undress(std::declval<T>()))>)
struct std::hash<T>
{
    std::size_t operator()(T const & v) const noexcept { ... }
};
```

**Pros:** Correct for all types, opt-in, extensible.
**Cons:** Two specializations in the preamble, requires maintaining the trait. Partial specialization of `std::hash` with overlapping constraints may have issues.

### Option 4: Always constexpr (let the compiler sort it out)

Mark `operator()` as `constexpr` unconditionally. In C++20, a `constexpr` function that can't actually be evaluated at compile time is just treated as a regular function -- it's not an error unless someone actually tries to use it in a constant expression.

```cpp
template <typename T>
    requires atlas::is_atlas_type<T>
struct std::hash<T>
{
    constexpr std::size_t operator()(T const & v) const noexcept
    {
        return std::hash<std::remove_cvref_t<decltype(atlas::undress(v))>>{}(
            atlas::undress(v));
    }
};
```

**Key insight:** Since C++14, `constexpr` on a function is a *suggestion*, not a *requirement*. If `std::hash<std::string>::operator()` isn't constexpr, then `std::hash<MyStringWrapper>::operator()` simply won't be usable in constant expressions -- but it will compile and work fine at runtime.

**Pros:** Simplest possible solution. One preamble. Works correctly for all types. Types with constexpr-capable underlying hash get constexpr hash automatically. Types without just work at runtime.
**Cons:** Need to verify this actually works across all target compilers (GCC, Clang, MSVC). Some older compiler versions may reject `constexpr` functions that call non-constexpr functions even when not in a constant expression context -- but C++20 mode should handle this correctly.

### Option 5: `constexpr` with `noexcept` conditional

Similar to Option 4, but also make `noexcept` conditional:

```cpp
constexpr std::size_t operator()(T const & v) const
    noexcept(noexcept(std::hash<underlying_t>{}(atlas::undress(v))))
{ ... }
```

This is more correct but doesn't solve the constexpr question specifically.

## Recommendation

**Option 4 seems most promising** -- just mark it `constexpr` and let the compiler handle it. The C++20 rules for constexpr are relaxed enough that a constexpr function calling a non-constexpr function is valid (it just can't be evaluated at compile time). This needs verification across compilers.

If Option 4 doesn't work reliably, **Option 1 (always non-constexpr)** is the safe fallback.

## Files Involved

- Preamble generation: Need to find where the auto_hash preamble template lives
  - Likely in the template system under `src/lib/atlas/` -- search for `auto_hash` or the preamble template text
- ClassInfo.cpp: `process_specializations()` at line ~564 handles `hash` and `no-constexpr-hash` tokens
- ClassInfo.hpp: `hash_const_expr` field controls constexpr on per-type hash (irrelevant for auto preamble)

## Resolution

**Chosen approach: Option 1 (non-constexpr by default) with opt-in `constexpr_hash` setting.**

The implementation combines Option 1 as the safe default with a file-level opt-in for users who need constexpr hash:

1. **`constexpr_hash` defaults to `false`** — safe for all types including `std::string`.
2. **`constexpr_hash=true`** at file level or `--constexpr-hash=true` on CLI enables `constexpr` on the preamble `operator()`. Only appropriate when all types in the file wrap constexpr-hashable underlying types.
3. **`no-constexpr-hash` is deprecated** with a warning: "has no effect — hash is non-constexpr by default."
4. **Pre-C++20 support added** — a `hash_base<T>` helper class in the preamble, with per-type one-liner specializations (`struct std::hash<MyType> : hash_base<MyType> {};`) guarded by `#if __cplusplus < 202002L`.

### Answers to the Questions

1. **Option 4 (always constexpr)** was rejected because `std::hash<std::string>` is not constexpr on any shipping standard library, and some compilers may reject the combination even in non-constexpr contexts.
2. **Constexpr hash is rarely used at compile time** in practice, confirming that non-constexpr default is the right call.
3. **`no-constexpr-hash` is fully deprecated** — it has no effect since hash is non-constexpr by default. Users who want constexpr hash use `constexpr_hash=true` at file level.
