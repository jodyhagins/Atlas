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

[type]
kind=struct
namespace=util
name=Counter
description=strong int; ++, --, out
```

Quick facts:
- `#` for comments (groundbreaking, I know)
- Each `[type]` section = one strong type
- All types share a header guard with SHA1 hash (collision-proof, even when you try)

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
strong double; +, -, *, /, <=>                      # Math-friendly double
strong std::string; ==, !=, out, no-constexpr-hash  # String with runtime hash
strong int; ++, --, bool, #<iostream>               # Counter with explicit include
strong std::vector<int>; ==, [], iterable           # Iterable container wrapper
strong std::string; ==, cast<std::string_view>      # String with explicit cast to view
strong int; ==, implicit_cast<bool>                 # Int with implicit bool conversion
```