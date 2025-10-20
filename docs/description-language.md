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
| `<=>` | Three-way comparison (adds `<compare>`) |
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
