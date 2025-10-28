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
atlas --interactions=true --input=interactions.txt --cpp-standard=20
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
