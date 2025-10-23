# Description Language

Syntax: `strong <type>; <option>, <option>, ...`

It's like a DSL, but you won't need a PhD to understand it.

## File Input Format

For when you have more than one type (which is always):

```
guard_prefix=MY_APP_TYPES    # optional, but recommended

[type]
kind=struct
namespace=math
name=Distance
description=strong int; +, -, ==, <=>

[struct util::Counter]
description=int; ++, --, out
```

Quick facts:
- `#` for comments (groundbreaking, I know)
- Each `[type]` section = one strong type
- All types share a header guard with SHA1 hash (collision-proof, even when you try)
- The `strong` keyword in descriptions is optional (see "Optional Strong Keyword" section below)

## Optional Strong Keyword

The `strong` keyword in descriptions is now optional. Both of these are equivalent:

```
description=strong double; +, -, *, /
description=double; +, -, *, /
```

This makes descriptions more concise while still maintaining clarity. The keyword is purely optional for backwards compatibility and readability preference.

## Global Namespace Support

Types can be declared in the global namespace by using `::` as the namespace:

```
[type]
namespace=::
name=GlobalCounter
description=int; ++, --, ==
```

This generates the type at global scope instead of in a C++ namespace (this is a no-judgement zone).

## Inline Type Name Syntax

For in-file definitions, you can use an inline syntax that combines the name and type:

```
[type]
kind=struct
namespace=foo
name=Bar
description=int; ++, --, ==
```

The above can also be represented as:

```
[struct foo::Bar]
description=int; ++, --, ==
```

Both syntaxes are equivalent.

## Named Constants

Define named constants for your strong types, similar to scoped enum values:

```
[type]
kind=struct
namespace=math
name=Distance
description=double; +, -, ==, <=>
constants= zero : 0.0 ; epsilon : 1e-10
constants= infinity : std::numeric_limits<double>::infinity()
```

The constants section maps constant names to their values. Each constant generates a static member:
```cpp
static constexpr Distance zero = Distance(0.0);
static constexpr Distance infinity = Distance(std::numeric_limits<double>::infinity());
static constexpr Distance epsilon = Distance(1e-10);
```

When `no-constexpr` is specified, constants are generated as `static const` instead of `static constexpr`.

Constants are useful for providing well-known values and sentinel values for your strong types.

## C++ Standard Specification

Atlas can generate a `static_assert` to enforce a minimum C++ standard at compile time. This ensures that code using modern C++ features (like the spaceship operator) will produce clear error messages if compiled with an older standard.

### Three Ways to Specify the Standard

**1. Description-level** (lowest precedence):
```
[test::UserId]
description=strong int; <=>, c++20
```

**2. File-level** (medium precedence, applies to all types in file):
```
cpp_standard=20

[test::UserId]
description=strong int; <=>

[test::SessionId]
description=strong int; ==, !=
```

**3. Command-line** (highest precedence, overrides everything):
```bash
atlas --input=types.txt --cpp-standard=20
```

### Supported Standards

- `11` or `c++11` - C++11 (default, no static_assert generated)
- `14` or `c++14` - C++14
- `17` or `c++17` - C++17
- `20` or `c++20` - C++20
- `23` or `c++23` - C++23

### Generated Code

For C++20, Atlas generates:
```cpp
#ifndef MY_TYPES_GUARD
#define MY_TYPES_GUARD

static_assert(__cplusplus >= 202002L,
    "This file requires C++20 or later. Compile with -std=c++20 or higher.");

// ... rest of generated code ...
```

For C++11 (the default), no static_assert is generated since it's the minimum supported standard.

### Multi-Type Files

When multiple types in one file specify different standards, Atlas uses the **maximum** standard:

```
[test::Type1]
description=int; +, -, c++14

[test::Type2]
description=int; <=>, c++20

# Generated file will require C++20 (the max of 14 and 20)
```

## Profiles

Profiles let you define reusable feature bundles:

```
# Define profiles at file level (before any [type] sections)
profile=NUMERIC; +, -, *, /, ==, !=, <, <=, >, >=, hash
profile=COMPARABLE; ==, !=

[type]
kind=struct
namespace=math
name=Distance
description=strong double; {NUMERIC}, ->

[type]
kind=struct
namespace=util
name=Id
description=strong int; {COMPARABLE}, hash
```

Profile rules:
- Profile names must be `[a-zA-Z0-9_-]+`
- Define profiles with `profile=NAME; feature, feature, ...`
- Use profiles with `{NAME}` in descriptions
- Multiple profiles can be composed: `{FOO}, {BAR}, +`
- Features are deduplicated automatically
- Profiles must be defined before use

Example:
```
profile=ARITH; +, -, *, /, +=, -=, *=, /=
profile=CMP; ==, !=, <, <=, >, >=

[type]
description=strong int; {ARITH}, {CMP}, hash
# Expands to: +, -, *, /, +=, -=, *=, /=, ==, !=, <, <=, >, >=, hash
```

## Options

### Operators
| Option | Behavior |
|--------|----------|
| `+`, `-`, `*`, `/`, `%` | Arithmetic with `op=` and binary operators |
| `&`, `|`, `^`, `<<`, `>>` | Bitwise with `op=` and binary operators |
| `+*`, `-*` | Binary and unary versions |
| `u+`, `u-`, `u~`, `~` | Unary operators |
| `==`, `!=`, `<`, `<=`, `>`, `>=` | Comparison operators |
| `<=>` | Three-way comparison operator (C++20+) with automatic C++11/14/17 fallback to all six comparison operators |
| `!`, `not`, `&&`, `and`, `||`, `or` | Logical operators |
| `++`, `--` | Increment/decrement (pre and post) |

### Access
| Option | Behavior |
|--------|----------|
| `@` | `operator*` to access underlying value |
| `&of` | `operator&` returns `std::addressof(value)` |
| `->` | Pointer-like member access |
| `[]` | Subscript (supports C++23 multidimensional) |
| `()` | Nullary call returning wrapped value |
| `(&)` | Call operator invoking callable with value |

### Conversion & I/O
| Option | Behavior |
|--------|----------|
| `bool` | Explicit conversion to `bool` |
| `cast<Type>` | Explicit cast operator to `Type` (requires `static_cast`) |
| `explicit_cast<Type>` | Explicit cast operator to `Type` (alias for `cast<Type>`) |
| `implicit_cast<Type>` | Implicit cast operator to `Type` (reduces type safety, use sparingly) |
| `out` | `operator<<` for output streams |
| `in` | `operator>>` for input streams |
| `fmt` | `std::formatter` specialization for C++20 `std::format` (wrapped in feature test macro) |
| `hash` | `std::hash` specialization |
| `iterable` | Member `begin()/end()` for range-based for loops |
| `assign` | Template assignment operator allowing assignment from compatible types |

### Control
| Option | Behavior |
|--------|----------|
| `no-constexpr` | Remove `constexpr` from all operations |
| `no-constexpr-hash` | Remove `constexpr` from hash only |
| `#<header>` or `#"header"` | Explicit include directive |

## Default Values

Because sometimes zero isn't the right default:

Command line:
```bash
atlas --default-value=42 ...
```

Input file:
```
[type]
default_value=42
```

## Automatic Includes

Atlas is smart enough to figure out your includes (most of the time):
- `std::string` → `<string>`
- `std::vector` → `<vector>`
- `std::chrono::...` → `<chrono>`

When it guesses wrong, use `#<header>` to fix it.

## Examples

```
double; +, -, *, /, <=>                            # Math-friendly double (strong keyword optional)
std::string; ==, !=, out, no-constexpr-hash        # String with runtime hash
int; ++, --, bool, #<iostream>                     # Counter with explicit include
std::vector<int>; ==, [], iterable                 # Iterable container wrapper
std::string; ==, cast<std::string_view>            # String with explicit cast to view
int; ==, implicit_cast<bool>                       # Int with implicit bool conversion
double; {NUMERIC}, ->                              # Using a profile (defined elsewhere)
```

## Spaceship Operator (`<=>`) with Automatic C++17 Fallback

The spaceship operator `<=>` is a powerful C++20 feature that simplifies comparison operations. Atlas makes it work seamlessly across all C++ standards:

### How It Works

When you specify `<=>` in your type description:

**C++20 and later:**
```cpp
friend constexpr auto operator <=> (MyType const &, MyType const &) = default;
friend constexpr bool operator == (MyType const &, MyType const &) = default;
```
The compiler synthesizes all comparison operators (`<`, `<=`, `>`, `>=`, `==`, `!=`) from the spaceship operator.

**C++11/14/17 (automatic fallback):**
```cpp
friend constexpr bool operator < (MyType const & lhs, MyType const & rhs)
{ return lhs.value < rhs.value; }
// ... and five more comparison operators: <=, >, >=, ==, !=
```
Atlas automatically generates all six comparison operators that delegate to the underlying type.

### Benefits

- ✅ **Write once, run anywhere**: Same description works from C++11 through C++23+
- ✅ **Semantic equivalence**: Fallback operators provide the same behavior as spaceship for strong types
- ✅ **No manual work**: You get all six comparison operators without specifying them individually
- ✅ **Future-proof**: Automatically upgrades to native spaceship when you move to C++20+

### Example

```
[struct math::Distance]
description=double; +, -, <=>
```

This single description generates:
- **C++20+**: Spaceship operator with synthesized comparisons
- **C++17-**: All six comparison operators explicitly generated

### Note on Redundancy

If you specify `<=>` along with individual comparison operators (like `==`, `<`, etc.), Atlas will warn you that they're redundant. Just use `<=>` alone for the cleanest code:

```
# Good - concise and works everywhere
description=int; +, -, <=>

# Works but redundant - generates warnings
description=int; +, -, <=>, ==, <
```
