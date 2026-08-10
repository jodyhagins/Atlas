# Description Language

Syntax: `strong <type>; <option>, <option>, ...`

It's like a DSL, but you won't need a PhD to understand it.

## File Input Format

For when you have more than one type (which is always):

```
guard_prefix=MY_APP_TYPES    # optional, but recommended
auto_ostream=true

[type]
kind=struct
namespace=math
name=Distance
description=strong int; +, -, ==, <=>

[struct util::Counter]
description=int; ++, --
```

Quick facts:
- `#` for comments (groundbreaking, I know)
- Each `[type]` section = one strong type
- All types share a header guard with SHA1 hash (collision-proof, even when you try)
- The `strong` keyword in descriptions is optional (see "Optional Strong Keyword" section below)
- Repeated `description=` lines accumulate, just like `forward=` and `constants=`: each extra line adds more semicolon-separated segments (operator lists or `forward=`) to the same specification

### File-Level Directives

These directives appear before the first `[type]` section and apply to all types in the file:

| Directive | Behavior |
|-----------|----------|
| `guard_prefix=PREFIX` | Custom prefix for header guard |
| `cpp_standard=N` | Minimum C++ standard (11, 14, 17, 20, 23) |
| `profile=NAME; features` | Define a reusable feature profile |
| `auto_hash=true` | Generate `std::hash<T>` for all atlas types in the file (C++20 concept-constrained; pre-C++20 via per-type specializations) |
| `constexpr_hash=true` | Make `auto_hash` operator() constexpr (default: false). Only use when all types wrap constexpr-hashable underlying types (e.g., `int`, not `std::string`). |
| `auto_ostream=true` | Generate `operator<<` for all atlas types in the file (SFINAE-constrained) |
| `auto_istream=true` | Generate `operator>>` for all atlas types in the file (SFINAE-constrained) |
| `auto_format=true` | Generate `std::formatter<T>` for all atlas types in the file (C++20, requires `__cpp_lib_format`) |
| `auto_from_atlas_cast=true` | Enable cross-construction among the types declared in this file (each type's allowlist is seeded with its same-file siblings) |
| `borrowed_types=A, B, ...` | Declare view-like types as borrowing: each name gets an `atlas::borrowed_type` specialization so the rvalue lifetime guards treat conversions to it as borrowing. Multiple lines accumulate. Types defined in the same file are forward-declared automatically; other types must be declared before the generated header is included. |

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
atlas --input=types.atlas --cpp-standard=20
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

## Templates

Templates define reusable parameterized type patterns. Define once, instantiate multiple times with different underlying types.

### Defining Templates

```
[template Optional T]
kind=struct
description=strong std::optional<{T}>; <=>, bool
```

Template definitions support:
- `kind=` - struct or class (default: struct)
- `namespace=` - default namespace for instances
- `description=` - type pattern with `{PARAM}` placeholders (required)
- `default_value=` - default value pattern
- `constants=` - constant definitions
- `forward=` - forwarded member functions

Parameters may also declare defaults inline in the header (`[template Name T=value]`). See "Default Parameter Values" below.

### Instantiating Templates

```
# Auto-generated name: Optional_int
[use Optional int]

# Custom name with 'as' keyword
[use Optional double as OptionalDouble]

# Multiple parameters
[template Pair K V]
kind=struct
description=strong std::pair<{K}, {V}>; ==

[use Pair int std::string as IntStringPair]
```

### Instance Overrides

Instances can override template defaults:

```
[template GenericType T]
kind=struct
namespace=templates::base
description=strong {T}; ==

# Override namespace
[use GenericType int as CustomInt]
namespace=my::custom::ns

# Override kind
[use GenericType double as DoubleClass]
kind=class

# Add constants (merged with template constants)
[template BaseValue T]
constants=is_template:1

[use BaseValue int as EnhancedInt]
constants=special:42
# Result has both is_template and special constants
```

### Profiles in Templates

Templates can use profile references:

```
profile=NUMERIC; +, -, *, /

[template ArithmeticValue T]
kind=struct
description=strong {T}; {NUMERIC}, ==

[use ArithmeticValue long as ArithmeticLong]
# Gets all NUMERIC operators plus ==
```

### Default Parameter Values

Template parameters may declare default values. Any of these spellings work:

```
[template Bool T=bool]
[template Bool T =bool]
[template Bool T= bool]
[template Bool T = bool]
```

A defaulted parameter may be omitted at the use-site. This is the most concise way to define a family of types that all share the same underlying representation:

```
auto_hash=true
auto_ostream=true

[template Bool T=bool]
description=strong {T}; ==, bool
default_value=false

[use Bool as IsAdmin]      # T defaults to bool
[use Bool as IsActive]
[use Bool as IsVerified]

[use Bool int as IntFlag]  # default may still be overridden
```

Like in C++, once a parameter has a default, every parameter that follows it must also have one:

```
# OK
[template Pair K V=double]

# OK
[template Pair K=int V=double]

# ERROR: V has a default but L does not
[template Triple K V=double L]
```

The default value is opaque substitution text — it is not parsed or validated as a C++ type, so anything legal as a template argument is legal as a default. The same restriction that applies to template arguments applies to defaults: spaces inside the value will be split on, so write `T=std::pair<int,double>`, not `T=std::pair<int, double>`.

When a `[use]` instantiation omits all arguments and has no `as` clause, the auto-generated name reflects the *effective* arguments after defaults are filled in. So `[use Bool]` and `[use Bool bool]` both produce a type named `Bool_bool`.

#### Per-instance constants

Each `[use]` may attach its own constants. Instance constants are merged on top of any constants declared on the template, so a single `[template Bool T=bool]` definition can produce many domain-specific flags, each with its own named values:

```
[template Bool T=bool]
description=strong {T}; ==, bool
default_value=false
constants=true_value:true; false_value:false   # shared by every instance

[use Bool as IsAdmin]
constants=admin:true; non_admin:false          # IsAdmin-specific

[use Bool as IsActive]
constants=enabled:true; disabled:false         # IsActive-specific

[use Bool as IsVerified]
constants=verified:true; unverified:false; pending:false
```

Each generated type carries the union of its template's constants and its own:

```cpp
IsAdmin::admin;        IsAdmin::non_admin;     IsAdmin::true_value;     IsAdmin::false_value;
IsActive::enabled;     IsActive::disabled;     IsActive::true_value;    IsActive::false_value;
IsVerified::verified;  IsVerified::unverified; IsVerified::pending;
IsVerified::true_value; IsVerified::false_value;
```

Multiple constants on a single line are separated by `;`, not `,`.

### Template Argument Limitations

Template arguments are space-separated. For complex types containing spaces (like `std::pair<int, double>`), omit the space after commas:

```
# CORRECT - no space after comma
[use Wrapper std::pair<int,double> as PairWrapper]

# INCORRECT - space after comma breaks parsing
[use Wrapper std::pair<int, double> as PairWrapper]
```

## Member Function Forwarding

Member function forwarding allows strong types to selectively expose member functions from their wrapped type, maintaining type safety while providing convenient access to underlying functionality.

### Basic Syntax

**Inline in description:**
```
[type]
kind=struct
namespace=util
name=SafeString
description=std::string; forward=size,empty,clear; ==, !=
```

**Separate line:**
```
[type]
kind=struct
namespace=util
name=SafeString
description=std::string; ==, !=
forward=size,empty,clear
```

Both syntaxes are equivalent and can be used interchangeably.

### Member Function Aliasing

Rename forwarded member functions using `memfn` syntax:

```
[type]
kind=struct
namespace=util
name=StringWrapper
description=std::string; forward=size:length,empty:is_empty; ==, !=
```

Generated code:
```cpp
// Forwards std::string::size() as length()
auto length() const & noexcept(...) -> decltype(...);

// Forwards std::string::empty() as is_empty()
auto is_empty() const & noexcept(...) -> decltype(...);
```

### Const-Only Forwarding

Use the `const` keyword to forward only const overloads:

```
[type]
kind=struct
namespace=util
name=ImmutableString
description=std::string; forward=const,size,empty; ==, !=
```

This generates only const-qualified overloads, preventing modification of the underlying value through forwarded member functions.

### Multiple Forward Lines

You can specify multiple `forward=` lines to organize member function groups:

```
[type]
kind=struct
namespace=util
name=MyString
description=std::string; ==, !=
forward=size,empty,length
forward=clear,append
```

### Generated Code

Atlas generates perfect forwarding wrappers with:
- **Four ref-qualified overloads** (C++11-20): `const &`, `const &&`, `&`, `&&`
- **Single deducing this overload** (C++23+): `template<typename Self> ... (this Self&&)`
- **Preserved noexcept specifications**: Automatically matches underlying member function
- **SFINAE-friendly return types**: Uses `decltype` for return type deduction

Example for C++20:
```cpp
template <typename... Args>
constexpr auto size(Args&&... args) const &
    noexcept(noexcept(value.size(std::forward<Args>(args)...)))
    -> decltype(value.size(std::forward<Args>(args)...))
{
    return value.size(std::forward<Args>(args)...);
}

// ... 3 more overloads for const &&, &, &&
```

Example for C++23:
```cpp
template <typename Self, typename... Args>
constexpr auto size(this Self&& self, Args&&... args)
    noexcept(noexcept(std::forward<Self>(self).value.size(std::forward<Args>(args)...)))
    -> decltype(std::forward<Self>(self).value.size(std::forward<Args>(args)...))
{
    return std::forward<Self>(self).value.size(std::forward<Args>(args)...);
}
```

### Common Use Cases

**Container wrappers:**
```
description=std::vector<int>; forward=push_back,pop_back,size,empty; ==, iterable
```

**Smart pointer wrappers:**
```
description=std::unique_ptr<Data>; forward=get,reset; ->, @, bool
```

**String wrappers:**
```
description=std::string; forward=size:length,const,empty:is_empty,clear; ==, !=, hash
```

### Command-Line Support

Member function forwarding also works with the `--forward` flag:

```bash
atlas --kind=struct --namespace=util --name=SafeString \
      --description="std::string; ==, !=" \
      --forward="size,empty,clear"
```

Multiple `--forward` flags accumulate:
```bash
atlas --kind=struct --namespace=util --name=MyString \
      --description="std::string; ==, !=" \
      --forward="size,empty" \
      --forward="clear"
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
| `cast<Type>` | Explicit cast operator to `Type` (requires `static_cast`). `const &`-qualified: if `Type` borrows the underlying's storage (e.g. `std::string` → `std::string_view`), converting a temporary is a compile error — bind the source to a named variable first. |
| `explicit_cast<Type>` | Explicit cast operator to `Type` (alias for `cast<Type>`) |
| `implicit_cast<Type>` | Implicit cast operator to `Type` (reduces type safety, use sparingly). Subject to the same rvalue borrow guard as `cast<Type>`. |
| `out` | **Deprecated.** Use `auto_ostream=true` at file level. (Still parses; enables `auto_ostream` for ALL types in the file.) |
| `in` | **Deprecated.** Use `auto_istream=true` at file level. (Still parses; enables `auto_istream` for ALL types in the file.) |
| `fmt` | **Deprecated.** Use `auto_format=true` at file level. (Still parses; enables `auto_format` for ALL types in the file.) |
| `hash` | **Deprecated.** Use `auto_hash=true` at file level. (Still parses; enables `auto_hash` for ALL types in the file.) |
| `iterable` | Member `begin()/end()` for range-based for loops |
| `assign` | Template assignment operator allowing assignment from compatible types |

### Construction
| Option | Behavior |
|--------|----------|
| `from_atlas_cast<A, B, ...>` | Constructor accepting the listed atlas types, or types that drill to one of them, via `atlas::cast`. A bare `from_atlas_cast` (no list) is a generation-time error — name the permitted types, or use `auto_from_atlas_cast=true` at file level for a same-file family. |

### Control
| Option | Behavior |
|--------|----------|
| `no-constexpr` | Remove `constexpr` from all operations |
| `no-constexpr-hash` | **Deprecated.** Has no effect — hash is non-constexpr by default. Use `constexpr_hash=true` at file level to opt in. |
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

Atlas automatically discovers most required standard headers for your types:

**Exact type matches:**
- `std::string` → `<string>`
- `std::vector` → `<vector>`
- `std::map` → `<map>`

**Integral type families:**
- `int8_t`, `uint32_t`, `int64_t` → `<cstdint>`
- `size_t`, `ptrdiff_t` → `<cstddef>`

**Namespace-qualified types:**
- `std::chrono::*` → `<chrono>`
- `std::filesystem::*` → `<filesystem>`
- `std::ranges::*` → `<ranges>`

**Complex nested types:**
- `std::map<std::string, std::int64_t>` → `<cstdint>`, `<map>`, `<string>`
- `std::vector<std::vector<int>>` → `<vector>`

When Atlas guesses wrong or you need a non-standard header, use `#<header>` to explicitly add it.

## Examples

```
double; +, -, *, /, <=>                            # Math-friendly double (strong keyword optional)
std::string; ==, !=, no-constexpr                  # String (use auto_hash/auto_ostream at file level)
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

## Arithmetic Modes

Atlas supports three arithmetic overflow handling modes for integer types:

### checked (Overflow Detection)

Arithmetic operations that would overflow throw `atlas::OverflowError`:

```
[struct math::SafeCounter]
description=int; +, -, *, checked
```

Generated code checks for overflow before performing operations:

```cpp
SafeCounter a{INT_MAX};
SafeCounter b{1};
auto c = a + b;  // Throws atlas::OverflowError
```

### saturating (Clamp to Limits)

Arithmetic operations that would overflow saturate to type limits:

```
[struct audio::Volume]
description=uint8_t; +, -, saturating
```

Generated code clamps results to min/max values:

```cpp
Volume a{255};
Volume b{10};
auto c = a + b;  // Result is 255 (saturated at max)
```

### wrapping (Modular Arithmetic)

Arithmetic operations that overflow wrap around (two's complement):

```
[struct crypto::Counter]
description=uint32_t; +, -, wrapping
```

Generated code performs modular arithmetic:

```cpp
Counter a{UINT32_MAX};
Counter b{1};
auto c = a + b;  // Result is 0 (wrapped around)
```

### Mode Comparison

| Mode | Overflow Behavior | Use Case |
|------|------------------|----------|
| **checked** | Throws exception | Financial calculations, safety-critical code |
| **saturating** | Clamps to limits | Audio/video processing, UI controls |
| **wrapping** | Wraps around | Hash functions, cryptography, intentional modular arithmetic |

### Default Behavior

Without a mode specified, arithmetic uses unchecked operations (standard C++ behavior):

```
description=int; +, -, *  # No checking, standard overflow behavior
```

## Cross-Type Construction (`from_atlas_cast<A, B, ...>`)

Atlas strong types have no implicit conversions by design. This means you can't construct one atlas type from another, even if they share the same underlying type:

```cpp
TcpBindAddress addr{UdpBindAddress{...}};  // Error: no matching constructor
```

The `from_atlas_cast<A, B, ...>` feature adds an opt-in constructor for the *named* types, drilling through the wrapping chain via `atlas::cast`:

```
[struct net::TcpBindAddress]
description=std::string; from_atlas_cast<net::UdpBindAddress>, ==, hash

[struct net::UdpBindAddress]
description=std::string; from_atlas_cast<net::TcpBindAddress>, ==, hash
```

Now cross-type construction works — but only between the listed types:
```cpp
UdpBindAddress udp{"192.168.1.1:8080"};
TcpBindAddress tcp{udp};       // OK: UdpBindAddress is listed
TcpBindAddress bad{HostName{"x"}};  // Error: HostName is not listed, even
                                    // though it also wraps std::string
```

A bare `from_atlas_cast` with no type list is a generation-time error: it
cannot express which types are related, so it silently accepted *every* type
sharing the underlying representation.

### How It Works

The generated constructor uses SFINAE to accept only arguments that are, or drill to, one of the named types:

```cpp
// Accepts an atlas type ArgT where:
// 1. ArgT is an atlas type (has atlas_value_type)
// 2. ArgT is not the same type (copy/move ctors handle that)
// 3. ArgT is not directly constructible to underlying (variadic ctor handles that)
// 4. ArgT is, or drills to, one of the listed types (an actual layer of its
//    chain -- being constructible from the same underlying does not qualify)
// 5. The cast the constructor performs is valid for the argument's value
//    category (see the lifetime note below)
template <typename ArgT, typename = /* SFINAE guards */>
constexpr explicit TcpBindAddress(ArgT && arg);
```

Value categories are preserved: lvalue sources copy, rvalue sources move
(layer by layer for nested types).

Named types are emitted into the constraint as written. Types declared later
in the same file are forward-declared automatically; types from other files
must be declared before the generated header is used. Note the consequence
for *mutual* allowlists across two files: each header would need the other's
type declared first, so no include order works on its own. Keep mutually
listed types in one `.atlas` file, or write a small header that
forward-declares them and include it before either generated header.

**Lifetime note:** when the constructed type's underlying *borrows* (e.g. a
`std::string_view`-backed type listing a `std::string`-backed source),
construction from an lvalue borrows its storage, but construction from a
temporary would store a dangling view — it is rejected at compile time. Bind
the source to a named variable first.

### File-Level Directive

To enable cross-construction among all the types declared in one file:

```
auto_from_atlas_cast=true

[struct net::TcpBindAddress]
description=std::string; ==, hash

[struct net::UdpBindAddress]
description=std::string; ==, hash
```

The directive is scoped to the file: each type's allowlist is seeded with its
same-file siblings, and a type with the same underlying declared in another
file is still rejected unless named in an explicit `from_atlas_cast<...>`
list. A type combining the directive with an explicit list accepts the union
of both sets.

### Multi-Level Drilling

Arguments that *drill to* a listed type are accepted. If `WrappedMillis`
wraps `Milliseconds` and only `Milliseconds` is listed, construction from a
`WrappedMillis` works — `Milliseconds` is an actual layer of its chain:

```cpp
Milliseconds ms{42};
WrappedMillis wrapped{ms};    // WrappedMillis wraps Milliseconds
SpawnTimeoutMs t{wrapped};    // OK: drills to the listed Milliseconds
```

The reverse does not hold: a type merely *constructible from* what the
argument drills down to (e.g. any other `int64_t`-backed type) is not
accepted.

## Rvalue Borrow Guards (Lifetime Safety)

A conversion is dangerous when the *target borrows* storage that the
*source owns*: `std::string` → `std::string_view` from a temporary is a
use-after-free. Every generated type carries a deleted, rvalue-only
conversion guard that turns this into a compile error on every route —
`static_cast`, copy-initialization, implicit conversion, `atlas::cast`
drilling, and the `from_atlas_cast` constructor:

```cpp
UserId make_id();

std::string_view sv1 = static_cast<std::string_view>(make_id()); // error
std::string_view sv2 = make_id();                 // error (implicit_cast)
auto sv3 = atlas::cast<std::string_view>(make_id());             // error

UserId id = make_id();                             // the remedy:
std::string_view sv = id;                          // bind a named variable
```

The guard is a two-type predicate — `atlas::borrow_policy<Target,
Underlying>` is true when the target borrows and the underlying owns — so it
is inert everywhere else: view-backed and scalar-backed types convert freely
from temporaries, and owning targets (`cast<std::string>`) still move.

Classification comes from three user-extensible traits in the shared
preamble (identical at every C++ standard):

- `atlas::borrowed_type<T>` — specialize to `std::true_type` for any type
  that refers to storage it does not own. Seeded for
  `std::basic_string_view` and `std::span`. Works at C++11.
- `atlas::is_borrowed<T>` — the query; where `<ranges>` exists it also
  honors `std::ranges::enable_borrowed_range<T>`, so C++20 view types are
  classified without mentioning Atlas. An Atlas-*generated* type is
  classified by what it wraps: a wrapper is borrowed when any layer beneath
  it is. Layering strong types on strong types therefore keeps working — a
  chain rooted in a view converts, drills, and constructs from temporaries
  at every depth (the view is copied out, nothing dangles), while a chain
  rooted in an owner stays guarded at every depth, including against
  drilling an expiring owner *into* a view-rooted wrapper. This recursion
  applies only to generated types (those deriving from
  `atlas::strong_type_tag`), whose accessor is a pure pass-through of the
  wrapped member. A hand-written type that adopts the
  `atlas_value_type`/`atlas_value_for` protocol may own storage and
  *compute* a view from it — classifying it by its declared value type
  would disarm the guards — so such types are never classified
  automatically and must opt in explicitly via `atlas::borrowed_type` or
  `enable_borrowed_range` when they genuinely borrow.
- `atlas::borrow_policy<Target, Underlying>` — the predicate every guard
  checks; specializable per pair in either direction (relax it for a backing
  that keeps storage alive independently, or tighten it for a pair nothing
  classified, such as a `char const *`-yielding handle).

### Declaring Third-Party View Types (`borrowed_types=`)

For third-party view types (or on projects below C++20), declare them at
file level and the generated header emits the `atlas::borrowed_type`
specializations for you:

```
borrowed_types=ext::StringRef, ext::BytesView

[struct api::Name]
description=std::string; ==, cast<ext::StringRef>, #"ext/string_ref.hpp"
```

Each specialization is individually include-guarded, so two generated
headers declaring the same type can coexist in one translation unit. The
named types must be declared before the generated header (use a `#"..."`
include token, as above) — except types defined in the same `.atlas` file,
which are forward-declared automatically ahead of the specializations, so a
file may declare its own view-backed types as borrowing.

**ODR rule for hand-written specializations:** a `borrowed_type` or
`borrow_policy` specialization feeds overload resolution inside the
generated types. If you write one by hand, it must be visible before any
generated header in *every* translation unit that includes one — visible in
one TU and not another is an ODR violation (no diagnostic required). The
`borrowed_types=` directive and a shared header are the reliable ways to
guarantee this.

**One mechanism per type:** do not combine a hand-written
`atlas::borrowed_type` specialization and a `borrowed_types=` declaration
for the same type in one translation unit. The generated specializations
are include-guarded against *each other*, but a hand-written one sits
outside that guard, and the pair is a redefinition error. Pick one
mechanism per type; prefer the directive.

**Declare before first use:** the specialization must also be visible
before anything instantiates `borrow_policy` for that type — including
code in another generated header. A generated header that declares
`borrowed_types=` for a type used by earlier headers must be included
before that use, or the declaration must be made in every file that names
the type; an explicit specialization that appears after the trait has
already been implicitly instantiated makes the program ill-formed.

### What Is Deliberately Not Guarded

`forward=` accessors and `iterable`'s `begin()`/`end()` keep the underlying
type's value-category behavior: `std::printf("%s", make_id().c_str())` and
range-`for` over a temporary work exactly as they do on the underlying type.
Guarding them would make the wrapper stricter than the type it wraps. The
rule: Atlas guards what it invents (conversions, drilling); it stays
faithful where you explicitly ask for the underlying's own member. The
residual hazard — storing such a pointer or iterator past the full
expression — is inherited from the underlying type.

Also out of scope: `std::span<T const>{temporary}` binds through span's own
range constructor and cannot be intercepted from the source side (the same
is true of `std::vector`), and mutable views like `std::span<std::byte>`
are not produced by the `const &`-qualified conversions in the first place.

## Constrained Types

Constrained types enforce invariants at construction time and after operations, preventing invalid states from ever existing.

### Built-in Constraints

Atlas provides seven built-in constraint types:

| Constraint | Syntax | Meaning | Example Use Case |
|------------|--------|---------|------------------|
| `positive` | `positive` | value > 0 | Prices, speeds, counts that must be positive |
| `non_negative` | `non_negative` | value >= 0 | Distances, ages, quantities that can be zero |
| `non_zero` | `non_zero` | value != 0 | Denominators, divisors, non-zero factors |
| `bounded<Min,Max>` | `bounded<0,100>` | Min <= value <= Max (closed interval) | Percentages, volume levels, bounded ranges |
| `bounded_range<Min,Max>` | `bounded_range<0,100>` | Min <= value < Max (half-open interval) | Array indices, iterator-like values |
| `non_empty` | `non_empty` | !value.empty() | Required usernames, non-empty collections |
| `non_null` | `non_null` | value != nullptr (or bool conversion) | Required pointers, handles that must be valid |

### Constraint Syntax

Add constraint keywords to your type description:

```
[struct audio::Volume]
description=int; bounded<0,100>, ==, <=>

[struct physics::Speed]
description=double; positive, +, -, *, /

[struct auth::Username]
description=std::string; non_empty, ==, !=

[struct sys::Handle]
description=void*; non_null, ==, !=

[struct math::Percentage]
description=int; bounded<0,100>, +, -, <=>, checked
```

### Constraint Checking

**Compile-time checking**: When values are `constexpr`, constraint violations cause compilation errors:

```cpp
// In generated code
constexpr Volume max_volume{100};     // OK
constexpr Volume invalid_volume{101}; // Compilation error: constraint violated
```

**Runtime checking**: For non-constant values, violations throw `atlas::ConstraintError`:

```cpp
int user_input = get_user_input();
try {
    Volume v{user_input};  // Validates at construction
    // Use v safely - guaranteed to be in [0, 100]
} catch (atlas::ConstraintError const & e) {
    std::cerr << "Invalid input: " << e.what() << '\n';
    // Output: "Volume: 150 violates constraint: value must be in [0, 100]"
}
```

### Constraints with Arithmetic Operations

Constrained types re-validate after arithmetic operations:

```cpp
Volume a{50};
Volume b{60};
auto c = a + b;  // Throws atlas::ConstraintError - result is 110, exceeds max of 100
```

This prevents invalid states from being created through operations:

```cpp
// All operations maintain the constraint
Volume v{50};
v += Volume{30};  // OK, result is 80
v += Volume{30};  // Throws - result would be 110
```

### Composing Constraints with Arithmetic Modes

Constraints work with all arithmetic modes (`checked`, `saturating`, `wrapping`):

```
[struct BoundedChecked]
description=uint8_t; bounded<0,100>, +, -, checked

[struct BoundedSaturating]
description=uint8_t; bounded<0,100>, +, -, saturating

[struct BoundedWrapping]
description=uint8_t; bounded<0,100>, +, -, wrapping
```

**Interaction behavior**:

- **checked + constraint**: Overflow check happens first, then constraint check
  ```cpp
  BoundedChecked a{50};
  BoundedChecked b{60};
  auto c = a + b;  // Throws on overflow OR constraint violation
  ```

- **saturating + constraint**: Saturation happens first, then constraint check on saturated result
  ```cpp
  BoundedSaturating a{255};  // Throws - 255 > 100
  BoundedSaturating b{90};
  auto c = b + b;            // Saturates to 100, passes constraint
  ```

- **wrapping + constraint**: Wrapping happens first, then constraint check on wrapped result
  ```cpp
  BoundedWrapping a{90};
  BoundedWrapping b{20};
  auto c = a + b;  // Wraps to 110, then throws - violates bounded<0,100>
  ```

### The `non_empty` Constraint

Types with `non_empty` constraint cannot be default-constructed (an empty value would violate the constraint):

```
[struct auth::Username]
description=std::string; non_empty, ==, !=
```

Generated code:

```cpp
struct Username {
    // Default constructor is DELETED
    Username() = delete;

    // Must construct with a value
    explicit constexpr Username(std::string const & v) : value(v) {
        if (!atlas::constraints::non_empty<std::string>::check(value)) {
            throw atlas::ConstraintError(
                "Username: violates constraint: value must not be empty");
        }
    }

    std::string value;
};
```

Usage:

```cpp
Username u;              // ERROR - won't compile (default ctor deleted)
Username u{""};          // Throws atlas::ConstraintError
Username u{"alice"};     // OK
```

### The `non_null` Constraint

Types with `non_null` constraint also delete the default constructor and work with any type that has `operator bool`:

```
[struct sys::ResourceHandle]
description=void*; non_null, ==, !=

[struct util::RequiredPtr]
description=std::shared_ptr<Data>; non_null, ->, @
```

Generated code validates using the type's boolean conversion:

```cpp
struct ResourceHandle {
    ResourceHandle() = delete;  // Cannot default-construct

    explicit constexpr ResourceHandle(void* v) : value(v) {
        if (!atlas::constraints::non_null<void*>::check(value)) {
            throw atlas::ConstraintError(
                "ResourceHandle: violates constraint: value must not be null");
        }
    }

    void* value;
};
```

Usage:

```cpp
ResourceHandle h;            // ERROR - won't compile
ResourceHandle h{nullptr};   // Throws atlas::ConstraintError
int x = 42;
ResourceHandle h{&x};        // OK
```

### Bounded Constraints: Closed vs Half-Open Intervals

Atlas provides two bounded constraint types:

**`bounded<Min,Max>`** - Closed interval [Min, Max]:
```
[struct Percentage]
description=int; bounded<0,100>, <=>
```
Accepts values where: `Min <= value <= Max`

**`bounded_range<Min,Max>`** - Half-open interval [Min, Max):
```
[struct ArrayIndex]
description=size_t; bounded_range<0,100>, <=>
```
Accepts values where: `Min <= value < Max`

Use `bounded_range` for iterator-like semantics where the upper bound is exclusive:

```cpp
// Container with 100 elements, valid indices are [0, 100)
ArrayIndex idx{0};    // OK
ArrayIndex idx{99};   // OK
ArrayIndex idx{100};  // Throws - out of range
```

### Error Messages

All constraints throw `atlas::ConstraintError` (inherits from `std::logic_error`) with descriptive messages:

```cpp
try {
    Volume v{150};
} catch (atlas::ConstraintError const & e) {
    std::cerr << e.what() << '\n';
    // Output: "Volume: 150 violates constraint: value must be in [0, 100]"
}

try {
    Username u{""};
} catch (atlas::ConstraintError const & e) {
    std::cerr << e.what() << '\n';
    // Output: "Username: violates constraint: value must not be empty"
}
```

**Error Message Formats**:

The error message format varies by context:

1. **Constructor violations**: `"{TypeName}: {value} violates constraint: {message}"`
2. **Arithmetic violations**: `"{TypeName}: arithmetic result violates constraint ({message})"`
3. **Forwarded function violations**: `"{TypeName}::{function}: operation violates constraint ({message})"`

Note: The implementation has minor formatting inconsistencies - most arithmetic operations include a space before the parenthesis, but a few do not. Do not rely on exact message formatting for error parsing.

### Constraints with Forwarded Member Functions

When using member function forwarding with constrained types, Atlas generates RAII-based constraint guards for non-const member functions to perform post-condition checking:

```cpp
// For a non_empty type with forwarded member functions:
template <typename... Args>
constexpr auto clear(Args&&... args) &
{
    using atlas::constraints::constraint_guard;
    [[maybe_unused]] auto guard = constraint_guard<atlas_constraint>(
        value,
        "Username::clear");
    return value.clear(std::forward<Args>(args)...);
}
```

**Key behaviors**:
- The guard validates the constraint in its **destructor** (after the operation completes)
- Uses `std::uncaught_exceptions()` to avoid throwing during stack unwinding (exception-safe)
- Const member functions do not include guards (cannot violate constraints by definition)
- If the operation would violate the constraint, throws `atlas::ConstraintError` with the forwarded function format

This enables safe forwarding of mutating operations while maintaining constraint guarantees. For more details, see the [API Reference](api-reference.md#constraint-guard).

### Constraint Predicates Reference

All constraint predicates are in the `atlas::constraints` namespace and generated types include a typedef:

```cpp
struct Volume {
    using atlas_constraint = atlas::constraints::bounded<int, 0, 100>;
    // ...
};

struct Speed {
    using atlas_constraint = atlas::constraints::positive<double>;
    // ...
};

struct Username {
    using atlas_constraint = atlas::constraints::non_empty<std::string>;
    // ...
};
```

Available predicates:
- `atlas::constraints::positive<T>`
- `atlas::constraints::non_negative<T>`
- `atlas::constraints::non_zero<T>`
- `atlas::constraints::bounded<T, Min, Max>`
- `atlas::constraints::bounded_range<T, Min, Max>`
- `atlas::constraints::non_empty<T>`
- `atlas::constraints::non_null<T>`

### Practical Examples

**Audio volume control (0-100 with saturation)**:
```
[struct audio::Volume]
description=uint8_t; bounded<0,100>, +, -, <=>, saturating
```

**Financial price (must be positive)**:
```
[struct finance::Price]
description=double; positive, +, -, *, /, <=>
```

**Network port (1024-65535 for user ports)**:
```
[struct net::ServerPort]
description=uint16_t; bounded<1024,65535>, ==, <=>
```

**Safe division (non-zero denominator)**:
```
[struct math::Denominator]
description=int; non_zero, *, /
```

**Required configuration string**:
```
[struct config::ApiKey]
description=std::string; non_empty, ==, hash
```

**Temperature with absolute zero limit**:
```
[struct physics::Temperature]
description=double; bounded<-273.15,1000000>, +, -, <=>
```

**Checked and bounded (double safety)**:
```
[struct SafePercentage]
description=uint8_t; bounded<0,100>, +, -, checked
```

### Best Practices

1. **Choose the right constraint**: `positive` vs `non_negative` matters - be precise
2. **Validate at boundaries**: Construct constrained types at API entry points
3. **Use with arithmetic modes**: Combine constraints with `checked`/`saturating` for enhanced safety
4. **Handle exceptions appropriately**: Catch `atlas::ConstraintError` at API boundaries
5. **Prefer `constexpr`**: When possible, use constexpr construction for compile-time validation
6. **Document behavior**: Note which operations might violate constraints in your API docs

### Common Pitfalls

**Pitfall 1: Arithmetic producing invalid values**
```cpp
Percentage a{50};
Percentage b{60};
auto c = a + b;  // THROWS - result is 110, exceeds max of 100
```
Solution: Use saturating arithmetic or validate operation results.

**Pitfall 2: Confusing positive with non_negative**
```cpp
// WRONG - allows zero
struct PositiveCount : non_negative<int> {};

// CORRECT
struct PositiveCount : positive<int> {};
```

**Pitfall 3: Forgetting deleted default constructor**
```cpp
Username u;  // ERROR - won't compile (non_empty deletes default ctor)
Username u{"alice"};  // Correct
```

## C++ Standard for Interaction Files

Interaction files also support C++ standard specification, working identically to strong type files.

### File-Level Specification

```
guard_prefix=MY_INTERACTIONS
cpp_standard=20

namespace=physics
Distance * Time -> Velocity
```

### Command-Line Override

```bash
atlas --interactions=true --input=interactions.atlas --cpp-standard=20
```

The `cpp_standard` directive and `--cpp-standard` flag work exactly as they do for strong type files:
- Defaults to C++11 (no assertion)
- Supported values: 11, 14, 17, 20, 23
- Generates `static_assert` immediately after header guard
- CLI flag overrides file-level specification

**Example generated code for C++20:**
```cpp
#ifndef MY_INTERACTIONS_HASH
#define MY_INTERACTIONS_HASH

static_assert(__cplusplus >= 202002L,
    "This file requires C++20 or later. Compile with -std=c++20 or higher.");

// ... rest of generated code ...
```

## Runtime Utilities

Atlas provides runtime utilities for working with generated strong types. These are callable objects that help extract underlying values when absolutely necessary.

> **Warning**: These utilities exist for interop scenarios where you *must* access raw values. Using them frequently defeats the purpose of strong types. If you find yourself reaching for these often, reconsider your design. Code reviewers should always ask: "Why do we need to drill into this type here?"

### `atlas::unwrap`

Removes exactly **one layer** of wrapping from a strong type or enum.

```cpp
#include "MyTypes.hpp"  // Generated header includes utilities

// For atlas strong types: returns the immediately wrapped type
namespace math {
    // SimpleInt wraps int
    // NestedInt wraps SimpleInt
    // TripleNestedInt wraps NestedInt
}

math::TripleNestedInt x{math::NestedInt{math::SimpleInt{42}}};

auto & one_layer   = atlas::unwrap(x);      // NestedInt& (just the first layer)
auto & two_layers  = atlas::unwrap(one_layer);  // SimpleInt&
auto & three_layers = atlas::unwrap(two_layers); // int&

// For enums: returns the underlying type
enum class Status : int { Active = 1, Inactive = 0 };
auto raw = atlas::unwrap(Status::Active);   // int (value: 1)

// SFINAE: fails to compile for non-atlas/non-enum types
// atlas::unwrap(42);  // Error: int has no layer to unwrap
```

**Value categories are preserved:**
- Lvalue → returns reference to underlying
- Const lvalue → returns const reference
- Rvalue → returns by value (moves if applicable)

### `atlas::undress`

Recursively drills through **all layers** to the innermost raw type. The full monty - removes all clothing until you're left with a butt-naked primitive most of the time.

```cpp
math::TripleNestedInt x{math::NestedInt{math::SimpleInt{42}}};

auto & naked = atlas::undress(x);  // int& (drilled through ALL layers)

// Contrast with unwrap:
auto & clothed = atlas::unwrap(x); // NestedInt& (only removed jacket)

// Also handles non-atlas types (identity):
int y = 42;
auto & still_int = atlas::undress(y);  // int& (unchanged)
```

### `atlas::cast<TargetT>`

A semantic cast that starts with the wrapped type and drills down until a cast to `TargetT` succeeds.

```cpp
math::TripleNestedInt x{math::NestedInt{math::SimpleInt{42}}};

// Drill to find castable type
auto val = atlas::cast<int>(x);        // 42 - found int at the bottom
auto mid = atlas::cast<math::SimpleInt>(x);  // SimpleInt{42} - stopped at SimpleInt

// Reference casts (lvalue sources only)
auto & ref = atlas::cast<int &>(x);    // int& - reference to innermost
ref = 100;                              // Modifies the wrapped value

// Unlike unwrap/undress, cast has a TARGET type and finds the first
// level where static_cast<TargetT>(...) succeeds
```

Two lifetime rules make `cast` safe to use with temporaries:

- **Reference targets require lvalue sources.** `cast<int &>`, `cast<T const &>`,
  and `cast<T &&>` from a temporary would return a reference into a value
  that dies at the end of the expression, so they are compile errors. A
  reference target also only binds to an *actual layer* of the chain, never
  to a temporary materialized by a conversion.
- **Borrowing targets require lvalue sources when the layer owns.** Drilling
  a temporary `std::string`-backed type to `std::string_view` would produce
  a dangling view, so `atlas::cast<std::string_view>(make_id())` is a
  compile error — bind the source to a named variable first. Lvalue sources
  (`atlas::cast<std::string_view>(id)`) borrow safely, and value targets
  from temporaries (`atlas::cast<std::string>(make_id())`) move.

### Comparison Summary

| Utility | Behavior | Use When |
|---------|----------|----------|
| `unwrap` | Remove exactly ONE layer | Interop with code expecting the immediate wrapped type |
| `undress` | Remove ALL layers | Interop requiring the raw primitive (serialization, FFI) |
| `cast<T>` | Drill until castable to T | Need a specific intermediate type from nested wrappers |

### When to Use (Sparingly!)

These utilities are escape hatches, not everyday tools:

- **Serialization**: `atlas::undress(price)` to get the raw double for JSON/binary encoding
- **FFI boundaries**: Passing values to C APIs that expect primitives
- **Legacy interop**: Interfacing with code that predates your strong types
- **Debugging**: Inspecting raw values in logs (prefer `out` stream operator instead)

If you're using these inside your core domain logic, you've likely undermined your type safety. The whole point of strong types is to *prevent* mixing `Price` with `Quantity` - drilling down defeats that protection.
