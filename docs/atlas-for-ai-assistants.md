# Atlas Strong Types -- Reference for AI Assistants

Atlas generates C++ structs that wrap a single underlying value.
The wrapper is **opaque by default** -- no implicit conversions, no inherited operators.
Everything is explicitly opted in.

Atlas has no public training data. Rely on this reference, not on guesses.

## The One Rule

**No raw primitives in interfaces.** Every function parameter, return type, and public member with semantic meaning gets its own strong type. `undress`, `unwrap`, and `cast` at a call site are a design smell -- they mean the type is missing a feature or the API is wrong.

```cpp
// BROKEN: raw types leak semantic meaning
void place_order(int user_id, int product_id, double price, int qty);

// FIXED: types make the compiler enforce correctness
void place_order(UserId user, ProductId product, Price price, Quantity qty);
```

If you find yourself writing `atlas::undress(x)` inside domain logic, **stop and add the missing feature to the type** (an operator, a forwarded member function, an interaction, or a file-level directive). The only legitimate uses of `undress`/`cast` are at system boundaries: serialization, FFI, legacy interop.

---

## Defining Types

### In a `.atlas` file

```
auto_hash=true
auto_ostream=true

[struct app::UserId]
description=int; ==, <=>

[struct app::SessionId]
description=int; ==, <=>
```

File-level directives (`auto_hash`, `auto_ostream`, etc.) apply to all types in the file.

### In CMakeLists.txt

```cmake
# Single type
atlas_add_type(UserId int "==, <=>" TARGET my_lib)

# Multiple types from file
add_atlas_strong_types_from_file(
    INPUT types.atlas
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/types_gen.hpp
    TARGET my_lib)

# Multiple types inline
add_atlas_strong_types_inline(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/types_gen.hpp
    TARGET my_lib
    CONTENT "
auto_hash=true

[struct app::UserId]
description=int; ==, <=>

[struct app::SessionId]
description=int; ==, <=>
")
```

---

## Feature Reference

### Operators

| Feature | Effect |
|---------|--------|
| `+`, `-`, `*`, `/`, `%` | Arithmetic binary + compound assignment |
| `+*`, `-*` | Binary + unary combined (`+*` = `+` + `u+`) |
| `&`, `\|`, `^`, `<<`, `>>` | Bitwise binary + compound assignment |
| `u+`, `u-`, `u~`, `~` | Unary |
| `++`, `--` | Pre/post increment/decrement |
| `==`, `!=` | Equality |
| `<`, `<=`, `>`, `>=` | Relational |
| `<=>` | Spaceship (all 6 comparisons; works C++11-23) |
| `!`, `&&`, `\|\|` | Logical |
| `->`, `@`, `()`, `(&)`, `[]`, `&of` | Access/pointer-like/callable |
| `bool` | Explicit `operator bool` |
| `iterable` | `begin()`/`end()` for range-for |
| `assign` | Template assignment from compatible types |

### Construction and Conversion

| Feature | Effect |
|---------|--------|
| `from_atlas_cast<A, B, ...>` | Constructor from the listed atlas types (or types drilling to one). Bare `from_atlas_cast` is a generation-time error. |
| `cast<Type>` | Explicit `operator Type()`, `const &`-qualified. If `Type` borrows the underlying's storage (e.g. `std::string_view` from `std::string`), converting a temporary is a compile error; bind a named variable first. |
| `implicit_cast<Type>` | Implicit `operator Type()` (rarely appropriate); same rvalue borrow guard |

### Constraints

| Constraint | Meaning |
|------------|---------|
| `positive` | value > 0 |
| `non_negative` | value >= 0 |
| `non_zero` | value != 0 |
| `bounded<Min,Max>` | Min <= value <= Max |
| `bounded_range<Min,Max>` | Min <= value < Max |
| `non_empty` | !value.empty() (deletes default ctor) |
| `non_null` | value != nullptr (deletes default ctor) |

### Arithmetic Modes

| Mode | Behavior |
|------|----------|
| `checked` | Throws `atlas::OverflowError` on overflow |
| `saturating` | Clamps to type limits |
| `wrapping` | Two's complement wrap-around |

### File-Level Directives

Set at the top of a `.atlas` file (before any `[type]` sections) or via CLI flags.

| Directive | Effect |
|-----------|--------|
| `auto_hash=true` | `std::hash<T>` for all types (C++20 concept-constrained; pre-C++20 via per-type specializations) |
| `constexpr_hash=true` | Make hash `operator()` constexpr (default: false; only for constexpr-hashable underlying types like `int`) |
| `auto_ostream=true` | `operator<<` for all types |
| `auto_istream=true` | `operator>>` for all types |
| `auto_format=true` | `std::formatter<T>` for all types (C++20) |
| `auto_from_atlas_cast=true` | Cross-type construction among the types declared in this file (same-file only; unions with explicit `from_atlas_cast<...>` lists) |
| `borrowed_types=A, B` | Declare third-party view-like types as borrowing for the rvalue lifetime guards (emits guarded `atlas::borrowed_type` specializations) |

### Member Function Forwarding

```
forward=size,empty,clear               # forward these member functions
forward=size:length,empty:is_empty     # with aliasing
forward=const,size,empty               # const-only access
forward=substr->TypeName               # return type transformation
```

### Named Constants

```
constants=zero:0.0; max:1000.0
```

Generates `static constexpr TypeName zero = TypeName(0.0);` etc.

### Control

| Feature | Effect |
|---------|--------|
| `no-constexpr` | Remove constexpr from all operations |
| `#<header>` | Add explicit `#include <header>` |
| `#"header"` | Add explicit `#include "header"` |

### Deprecated Tokens (Do Not Use)

| Token | Replacement |
|-------|-------------|
| `hash` | `auto_hash=true` at file level |
| `out` | `auto_ostream=true` at file level |
| `in` | `auto_istream=true` at file level |
| `fmt` | `auto_format=true` at file level |
| `no-constexpr-hash` | No replacement needed; hash is non-constexpr by default |

---

## Interactions (Cross-Type Operations)

Interactions define operators between **different** types. Without them, you'd have to unwrap both operands -- which violates the One Rule.

### Definition

```
# interactions.atlas
include "types_gen.hpp"

namespace=commerce

Price * Quantity -> Total        # one-directional
Price * int <-> Price            # commutative (both orderings)
Total / Price -> Quantity
Total / Quantity -> Price
MaxRetries <=> int <-> auto      # all six comparisons
```

A `<=>` interaction covers all six comparisons: C++20 rewrites `<`, `<=`, `>`,
`>=` from it, and Atlas generates `==` (which C++20 rewrites into `!=`) because
the language only synthesizes `==` from a *defaulted* `operator<=>`. Do not
write a paired `==` interaction unless you want different equality semantics
than `<=>` gives -- an explicit one replaces the generated one.

### CMakeLists.txt

```cmake
add_atlas_interactions_from_file(
    INPUT interactions.atlas
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/interactions_gen.hpp
    TARGET my_lib)
```

### Usage

```cpp
Total total = price * qty;              // interaction handles this
Quantity back = total / unit_price;     // and this
```

### Interaction Directives

| Directive | Purpose |
|-----------|---------|
| `include "header"` | Include header for type definitions |
| `namespace=ns` | Namespace for interactions that follow |
| `constexpr` / `no-constexpr` | Constexpr control |
| `concept=ConceptName T` | C++20 concept constraint for template interactions |
| `enable_if=expression` | C++17 SFINAE constraint |
| `value_access=atlas::undress` | How to extract values (default) |

---

## Using Types in Code

### Construction

```cpp
app::UserId id{42};                    // explicit only
app::UserId id2{};                     // default (value = 0, or default_value)
// app::UserId id3 = 42;              // ERROR: no implicit conversion
```

### Operators (when declared)

```cpp
math::Distance a{10}, b{3};
auto c = a + b;                        // Distance{13}
bool ok = a > b;                       // true (requires <=>)
```

### Constrained types

```cpp
audio::Volume v{150};                  // throws: bounded<0,100> violated
finance::Price p{-5.0};               // throws: positive violated
auth::Username u{""};                  // throws: non_empty violated
```

---

## Escape Hatches (System Boundaries Only)

Use these **only** at serialization, FFI, or legacy interop boundaries.

| Function | What it does | When to use |
|----------|-------------|-------------|
| `atlas::unwrap(x)` | Remove one wrapping layer | Nested types: get the immediate inner type |
| `atlas::undress(x)` | Remove all layers to raw value | Serialization, FFI |
| `atlas::cast<T>(x)` | Drill to a specific type | When you need a specific intermediate layer |

Return types: lvalue source gives a reference, rvalue source gives by value (prevents dangling).

Lifetime rules on `atlas::cast` (compile errors, not runtime hazards):
- Reference targets (`cast<T &>`, `cast<T const &>`, `cast<T &&>`) require an lvalue source.
- Borrowing targets from owning temporaries are rejected: `atlas::cast<std::string_view>(make_id())` does not compile. Bind a named variable (`auto id = make_id();`) and borrow from that.
- Owning targets from temporaries move: `atlas::cast<std::string>(make_id())` is fine.

| Trait | Answers |
|-------|---------|
| `atlas::is_borrowed<T>` | Does `T` borrow storage it does not own? (seeded for `string_view`/`span`, honors `enable_borrowed_range`, extensible via `atlas::borrowed_type`) |
| `atlas::borrow_policy<Target, Underlying>` | Would converting to `Target` borrow storage `Underlying` owns? (the predicate the guards check; per-pair specializable) |

---

## Traits and Concepts

Constraining generic code is **not** an escape hatch -- these are the intended way to write
templates over atlas types. Never hand-roll `atlas_value_type` detection; Atlas ships it.

| Name | Answers |
|------|---------|
| `atlas::is_atlas_type<T>` / `atlas::AtlasTypeC` | Is `T` an atlas type? |
| `atlas::holds_enum<T>` / `atlas::HoldsEnumC` | Is `T` an enum, or does it wrap one? |
| `atlas::unwrap_t<T>` | The type one layer down (SFINAE-friendly) |
| `atlas::undress_t<T>` | The root type; identity for non-atlas, non-enum types |
| `atlas::is_atlas_of<T, U>` / `atlas::AtlasOfC<T, U>` | Is `T` an atlas type rooted at exactly `U`? |
| `atlas::is_string_atlas<T>` / `atlas::StringAtlasC` | Is the root `std::string`? |
| `atlas::is_arithmetic_atlas<T>` / `atlas::ArithmeticAtlasC` | Is the root arithmetic (excluding `bool`)? |

All of them ignore cv-qualification and references on `T`. The `*C` concepts require C++20;
the traits work in every supported standard. The category concepts subsume `AtlasTypeC`, so
a `StringAtlasC` overload beats an `AtlasTypeC` overload rather than being ambiguous with it.

---

## Common Mistakes

### 1. Raw primitives in interfaces

```cpp
// WRONG
double calculate_total(double price, int qty);

// RIGHT -- define an interaction: Price * Quantity -> Total
Total calculate_total(Price price, Quantity qty) { return price * qty; }
```

### 2. Unwrapping instead of declaring features

```cpp
// WRONG -- undressing to compare
if (atlas::undress(a) < atlas::undress(b)) { ... }

// RIGHT -- add <=> to the type description
if (a < b) { ... }
```

### 3. Undressing inside domain logic

```cpp
// WRONG
bool is_expensive(Price p) { return atlas::undress(p) > 100.0; }

// RIGHT -- use constants and comparison operators
// description=double; <=>
// constants=expensive_threshold:100.0
bool is_expensive(Price p) { return p > Price::expensive_threshold; }
```

### 4. Raw public members

```cpp
// WRONG
struct Order {
    int user_id;
    double price;
};

// RIGHT
struct Order {
    UserId user_id;
    Price price;
};
```

### 5. Missing interactions for cross-type math

```cpp
// WRONG
Total compute(Price p, Quantity q) {
    return Total{atlas::undress(p) * atlas::undress(q)};
}

// RIGHT -- define interaction: Price * Quantity -> Total
Total compute(Price p, Quantity q) { return p * q; }
```

### 6. Type with no features

A type with zero features is useless. At minimum, most types need `==` and often `<=>`:
```
description=int; ==, <=>
```

### 7. Undress at call sites

```cpp
// WRONG -- caller is responsible for unwrapping
log_event(atlas::undress(user_id), atlas::undress(order_id));

// RIGHT -- the function should accept strong types
log_event(user_id, order_id);
// ... and the serialization layer handles unwrapping internally
```

---

## Decision Flowchart

When you need to do something with a strong type value:

1. **Can the type's declared operators handle it?** Yes -> use them.
2. **Could a new feature handle it?** Yes -> add the feature (`<=>`, `forward=...`, `auto_hash=true`, etc.).
3. **Is this a cross-type operation?** Yes -> define an interaction.
4. **Is this a system boundary?** Yes -> use `atlas::undress(x)` or `atlas::cast<T>(x)`.
5. **Everything else** is a design problem. Fix the types, not the call site.

---

## Complete Example

### types.atlas

```
guard_prefix=COMMERCE_TYPES
auto_from_atlas_cast=true
auto_hash=true
auto_ostream=true
auto_format=true

[struct commerce::Price]
description=double; +, -, <=>
constants=zero:0.0

[struct commerce::Quantity]
description=int; +, -, <=>
constants=zero:0

[struct commerce::Total]
description=double; +, -, <=>
constants=zero:0.0

[struct commerce::ProductId]
description=int; ==, <=>

[struct commerce::OrderId]
description=int; ==, <=>

[struct commerce::ProductName]
description=std::string; ==, no-constexpr
forward=size,empty,const
```

### interactions.atlas

```
include "types_gen.hpp"

namespace=commerce

Price * Quantity -> Total
Total / Price -> Quantity
Total / Quantity -> Price
Quantity * int <-> Quantity
Price * double <-> Price
```

### Usage

```cpp
#include "types_gen.hpp"
#include "interactions_gen.hpp"

commerce::Total calculate_order_total(
    commerce::Price unit_price,
    commerce::Quantity qty)
{
    return unit_price * qty;   // interaction handles this
}

void print_receipt(
    commerce::OrderId order_id,
    commerce::ProductName name,
    commerce::Price price,
    commerce::Quantity qty,
    commerce::Total total)
{
    // auto_ostream handles all types -- no unwrapping
    std::cout << "Order " << order_id << ": "
              << qty << "x " << name
              << " @ " << price
              << " = " << total << '\n';
}

// Serialization boundary -- unwrapping is appropriate HERE
void serialize_to_json(commerce::Price price, JsonWriter & w) {
    w.write("price", atlas::undress(price));
}
```
