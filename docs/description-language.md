# Description Language Reference

Atlas consumes descriptions of the form:

```
strong <underlying-type>; <option>, <option>, ...
```

- `strong <underlying-type>` defines the wrapped type, e.g., `strong std::string`.
- Each `<option>` enables extra behavior for the generated class.

## Input Modes

Atlas supports two input modes (use whichever fits your workflow):

1. **Command-line mode**: Specify a single type definition via command-line arguments (original behavior)
2. **File input mode**: Read multiple type definitions from an input file using `--input=<file>` (new feature)

### File Input Format

When using `--input=<file>`, the file uses a simple key=value format with `[type]` section markers:

```
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

**Key points:**
- `guard_prefix` is optional - if provided, the guard will be `PREFIX_SEPARATOR_SHA1`
- If no prefix is given, defaults to `ATLAS_SEPARATOR_SHA1` (ensures valid C++ identifier)
- Lines starting with `#` are treated as comments
- Empty lines are ignored
- Each `[type]` section defines one strong type
- **All types share a single header guard** computed from the combined content
- The SHA1 ensures the guard is unique to the exact content

**Example input file:**
```
# Application strong types
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

This generates a single header with guard like: `MY_APP_TYPES_A1B2C3D4E5F6...` (SHA1)

## Operator Options

| Option | Behavior |
|--------|----------|
| `+`, `-`, `*`, `/`, `%`, `&`, `|`, `^`, `<<`, `>>` | Adds compound assignment (`op=`) and binary friend operators. |
| `+*`, `-*` | Shorthand for both binary and unary versions (`+, u+`), (`-, u-`). |
| `u+`, `u-`, `u~`, `~` | Adds unary operators returning new strong types. |
| `==`, `!=`, `<`, `<=`, `>`, `>=` | Adds comparison friend operators. |
| `<=>` | Adds defaulted three-way comparison (requires `<compare>`). |
| `!`, `not` | Adds unary logical-not (`operator not`). |
| `&&`, `and` | Adds binary logical AND (`operator and`). |
| `||`, `or` | Adds binary logical OR (`operator or`). |
| `++`, `--` | Adds pre- and post-increment/decrement. |
| `@` | Provides `operator*` to access the underlying object. |
| `&of` | Provides `operator&` returning `std::addressof(value)` (adds `<memory>`). |
| `->` | Provides pointer-like member access (adds `<memory>`). |
| `bool` | Adds explicit conversion to `bool`. |
| `()` | Adds nullary call operator returning the wrapped value. |
| `(&)` | Adds call operator that invokes a callable with the wrapped value (adds `<utility>` and `<functional>`). |
| `[]` | Adds subscript operator that forwards to the wrapped object. Supports C++23 multidimensional subscripts. |
| `out` | Adds `operator<<` (adds `<ostream>`). |
| `in` | Adds `operator>>` (adds `<istream>`). |
| `hash` | Generates `std::hash` specialization for use in unordered containers (adds `<functional>`). |

## Constexpr Control Options

By default, all generated operations are marked `constexpr`, allowing use in constant expressions. You can opt out if needed:

| Option | Behavior |
|--------|----------|
| `no-constexpr` | Removes `constexpr` from all operations. Use when wrapping types that don't support constexpr operations. |
| `no-constexpr-hash` | Like `hash`, but omits `constexpr` from only the hash specialization. Everything else remains constexpr. Useful when wrapping types whose `std::hash` isn't constexpr (e.g., `std::string` in some stdlib implementations). |

**Examples:**
```
strong std::string; ==, hash              # All constexpr (may not compile with older stdlib)
strong std::string; ==, no-constexpr-hash # Constexpr ops, runtime hash
strong std::string; ==, hash, no-constexpr # Nothing constexpr
```

## Include Directives

Use `#<header>` or `#"path/to/header"` to force an include. Single quotes are normalized to double quotes, so `#'my/header.hpp'` is valid.

## Automatic Include Detection

Atlas attempts to include standard headers when it recognizes common standard library types. Examples:

- `std::string` → `<string>`
- `std::chrono::...` → `<chrono>`
- `std::vector<...>` → `<vector>`
- `std::optional<...>` → `<optional>`

See `StrongTypeGenerator.cpp` for full heuristics. When in doubt, add explicit `#<header>` options.

## Namespaces and Class Names

- `--namespace` supplies the enclosing namespace (e.g., `app::types`).
- `--name` may include scopes (`Outer::Inner::Type`); the final identifier becomes the class name.

Atlas trims redundant leading/trailing `:` characters when generating code.

## Validation

- `--kind` must be `struct` or `class`.
- Names must compose of letters, digits, underscores, or `::`; numeric-leading identifiers are rejected.
- Missing required flags result in descriptive errors.

## Hash Support

The `hash` option generates a `std::hash` specialization for your strong type, enabling use in `std::unordered_map`, `std::unordered_set`, and other hash-based containers:

```
strong int; ==, hash
```

The generated specialization:
- Appears outside the type's namespace but inside the header guard
- Delegates to `std::hash` of the underlying type
- Is conditionally `noexcept` based on the underlying type's hash function
- Requires the underlying type to be hashable

**Example usage:**
```cpp
// Generate with: strong int; ==, hash
std::unordered_set<MyStrongInt> ids;
ids.insert(MyStrongInt{42});

std::unordered_map<MyStrongInt, std::string> names;
names[MyStrongInt{1}] = "Alice";
```

## Subscript Operator Support

The `[]` option generates a subscript operator that forwards arguments to the wrapped object. The implementation provides:
- **Single-argument subscripting** (C++20 compatible) - always available
- **Multi-argument subscripting** (C++23) - enabled via `__cpp_multidimensional_subscript` feature test macro

```
strong std::vector<int>; []
strong std::map<std::string, int>; []
```

**Single-argument subscripting (C++20 compatible):**
```cpp
// strong std::vector<int>; []
Array arr{std::vector<int>{1, 2, 3, 4, 5}};
int x = arr[2];  // Returns 3
arr[0] = 42;     // Modifies element
```

**Multi-dimensional subscripting (C++23):**
```cpp
// strong MyMatrix; []
// (assuming MyMatrix supports multi-dimensional operator[])
Matrix m{/* ... */};
auto val = m[1, 2];     // C++23 syntax
m[0, 0] = 42;           // C++23 syntax
```

**Note:** The generated subscript operator will work with any type that supports `operator[]`, including containers like `std::vector`, `std::array`, `std::map`, raw arrays, and custom types. For C++23 multi-dimensional subscripts, the underlying type must support that syntax.

## Default Value Support

Default values for the default constructor are specified via the `--default-value` command-line parameter or `default_value=` in input files.

### Command-Line Usage:
```bash
atlas --kind=struct --namespace=example --name=Counter \
      --description="strong int; +, -, ==" \
      --default-value=42
```

### Input File Usage:
```
[type]
kind=struct
namespace=example
name=Counter
description=strong int; +, -, ==
default_value=42
```

This generates a type with member initialization and a defaulted constructor:

```cpp
struct Counter
{
    int value{42};
    constexpr explicit Counter() = default;
    // ...
};
```

**Examples:**
```bash
# Simple integer default
atlas ... --default-value=42

# String default (use quotes in shell)
atlas ... --default-value='"hello, world!"'

# Complex expression
atlas ... --default-value='std::vector<int>{1, 2, 3}'
```

## Constexpr Support

All generated functions are marked `constexpr` where possible, enabling compile-time evaluation and use in constant expressions.

**Functions NOT marked constexpr:**
- **Stream operators**: `in` and `out` (I/O operations cannot be constexpr)


**Example usage:**
```cpp
// strong int; +, -, *, ==, <=>
constexpr Distance d1{10};
constexpr Distance d2{20};
constexpr auto d3 = d1 + d2;  // Evaluated at compile time
static_assert(d3 == Distance{30});

constexpr int raw = static_cast<int>(d3);  // constexpr cast
static_assert(raw == 30);

// With default value (specified via --default-value=100)
// strong int; +, -
constexpr Score s1;  // Initialized to 100
static_assert(static_cast<int>(s1) == 100);
```

## Example

```
strong std::chrono::nanoseconds;
+, -, <=>, bool, out, #'fmt/chrono.h'
```

Generates arithmetic support, defaulted spaceship comparison, `bool` conversion, streaming operator, and manually includes `fmt/chrono.h`. All generated functions (except `out`) are constexpr.

