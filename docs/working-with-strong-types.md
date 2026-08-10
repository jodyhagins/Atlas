# Working with Atlas Strong Types

Atlas generates strong type wrappers that **prevent accidental mixing** of semantically distinct values.
A `Price` and a `Quantity` may both be `double` underneath, but the compiler will reject any attempt to use one where the other is expected.

This guide explains **how to work with values** inside strong types: how to avoid unwrapping them at all (preferred), and how to extract the raw value when you truly must.

## Table of Contents

- [Philosophy: No Naked Types](#philosophy-no-naked-types)
- [Opt-In Features That Eliminate Unwrapping](#opt-in-features-that-eliminate-unwrapping)
  - [Operators](#operators)
  - [Stream I/O](#stream-io)
  - [Hashing](#hashing)
  - [Formatting](#formatting)
  - [Member Function Forwarding](#member-function-forwarding-forward)
  - [Interactions (Cross-Type Operators)](#interactions-cross-type-operators)
  - [Direct Access Operators](#direct-access-operators---and-)
  - [Conversion Operators](#conversion-operators)
  - [Cross-Type Construction](#cross-type-construction-from_atlas_casta-b-)
- [When You Must Access the Raw Value](#when-you-must-access-the-raw-value)
  - [`atlas_value_for` (Hidden Friend)](#atlas_value_for-hidden-friend)
  - [`atlas::unwrap` (One Layer)](#atlasunwrap-one-layer)
  - [`atlas::undress` (All Layers)](#atlasundress-all-layers)
  - [`atlas::cast<T>` (Targeted Drilling)](#atlascastt-targeted-drilling)
- [Traits and Concepts](#traits-and-concepts)
  - [Is it an atlas type?](#is-it-an-atlas-type)
  - [What does it wrap?](#what-does-it-wrap)
  - [What kind of thing does it wrap?](#what-kind-of-thing-does-it-wrap)
- [Return Type Reference](#return-type-reference)
- [Best Practices](#best-practices)

---

## Philosophy: No Naked Types

The whole point of strong types is that raw primitives never appear in interfaces.
Every function parameter, return type, and public data member with semantic meaning gets its own type.

**The test is simple:** if you see `atlas::undress`, `atlas::unwrap`, or `atlas::cast` at a call site inside domain logic, something is wrong.
Either the type is missing a feature, or the API accepted a raw primitive where it should have accepted a strong type.

```cpp
// BAD: raw primitives in the interface
void place_order(int user_id, int product_id, double price, int qty);

// GOOD: types make the compiler enforce correctness
void place_order(UserId user, ProductId product, Price price, Quantity qty);
```

```cpp
// BAD: unwrapping to do something the type could do directly
if (atlas::undress(price_a) < atlas::undress(price_b)) { ... }
std::cout << atlas::undress(user_id);

// GOOD: declare the features and let the type work
if (price_a < price_b) { ... }   // requires: <=>
std::cout << user_id;             // requires: auto_ostream=true
```

```cpp
// BAD: raw members in a struct
struct Order { int user_id; double price; };

// GOOD: strong types everywhere
struct Order { UserId user_id; Price price; };
```

The escape hatches (`undress`, `unwrap`, `cast`) exist for **system boundaries only**: serialization, FFI, legacy interop.
Inside domain logic, they are a design smell.

The rest of this guide shows how to make the "GOOD" examples work.

---

## Opt-In Features That Eliminate Unwrapping

### Operators

The most common reason to unwrap is to do arithmetic or comparison.
Instead, declare the operators on the type:

```
[struct math::Distance]
description=double; +, -, *, /, <=>
```

Now `Distance` supports `+`, `-`, `*`, `/`, all six comparisons, and their compound assignment variants (`+=`, `-=`, etc.):

```cpp
Distance a{10.0}, b{3.0};
auto c = a + b;         // Distance{13.0}
bool closer = a < b;    // false
a -= b;                 // Distance{7.0}
```

See the [description language reference](description-language.md#operators) for the full operator table.

### Stream I/O {#stream-io}

Enable ostream/istream support at file level:

```
auto_ostream=true
auto_istream=true

[struct app::UserId]
description=int; ==, <=>
```

```cpp
UserId id{42};
std::cout << id;           // prints "42"
std::cin >> id;            // reads an int into the wrapper
```

> **Deprecated:** Per-type `out` and `in` tokens still work but are deprecated. They silently enable the corresponding `auto_*` preamble for the entire file (not just the annotated type). Use `auto_ostream=true` / `auto_istream=true` at file level instead.

### Hashing {#hashing}

Enable hash support at file level:

```
auto_hash=true

[struct app::SessionToken]
description=std::string; ==, no-constexpr
```

```cpp
std::unordered_set<SessionToken> tokens;
std::unordered_map<SessionToken, Connection> connections;
```

Atlas generates `std::hash` support for all atlas types in the file:
- **C++20+:** A concept-constrained partial specialization handles all types automatically.
- **Pre-C++20:** Per-type explicit specializations inheriting from a `hash_base<T>` helper.

By default, the hash `operator()` is not `constexpr`. If all types in the file wrap constexpr-hashable underlying types (e.g., `int`), you can opt in with `constexpr_hash=true`.

> **Deprecated:** Per-type `hash` and `no-constexpr-hash` tokens still work but are deprecated. Use `auto_hash=true` at file level instead. `no-constexpr-hash` has no effect — hash is non-constexpr by default.

### Formatting {#formatting}

Enable format support at file level:

```
auto_format=true

[struct app::UserId]
description=int; ==
```

```cpp
auto msg = std::format("User {} logged in", id);
```

Atlas generates a C++20 concept-constrained `std::formatter` partial specialization for all atlas types in the file, guarded by a `__cpp_lib_format` feature test.

> **Deprecated:** Per-type `fmt` token still works but is deprecated. It silently enables `auto_format` for the entire file. Use `auto_format=true` at file level instead.

### File-Level Auto-Enable Directives

If you have many types in a file that all need `hash`, `out`, `in`, or `fmt`, you can enable them globally instead of repeating them in every description:

```
auto_hash=true
auto_ostream=true
auto_format=true

[struct app::UserId]
description=int; ==, <=>

[struct app::SessionId]
description=int; ==, <=>
```

Both types automatically get `std::hash`, `operator<<`, and `std::formatter` without listing those features individually.

| Directive | Effect |
|-----------|--------|
| `auto_hash=true` | `std::hash<T>` for all atlas types in the file (C++20 concept-constrained; pre-C++20 via per-type specializations) |
| `constexpr_hash=true` | Make `auto_hash` operator() constexpr (default: false). Only for constexpr-hashable underlying types. |
| `auto_ostream=true` | `operator<<` for all atlas types in the file (SFINAE-constrained) |
| `auto_istream=true` | `operator>>` for all atlas types in the file (SFINAE-constrained) |
| `auto_format=true` | `std::formatter<T>` for all atlas types in the file (C++20, requires `__cpp_lib_format`) |
| `auto_from_atlas_cast=true` | Cross-type construction among the types declared in the file (same-file only; use `from_atlas_cast<...>` per type for cross-file) |

### Member Function Forwarding (`forward`)

When the underlying type has member functions you need, forward them selectively instead of unwrapping:

```
[struct net::Hostname]
description=std::string; ==, no-constexpr
forward=size,empty,const
```

```cpp
Hostname h{"example.com"};
if (h.empty()) { ... }
auto len = h.size();        // No unwrapping needed
```

Key features:
- **Aliasing**: `forward=size:length` renames `size()` to `length()`
- **Const-only**: `forward=const,size,empty` prevents mutation
- **Return type transformation**: `forward=substr->TypeName` wraps the return value in a strong type
- **Alias + transformation**: `forward=substr:sub->TypeName` combines both
- **Multiple lines**: Split into logical groups for readability

Return type transformation example:
```
[struct test::Username]
description=std::string; ==
forward=substr->Username
```

```cpp
Username u{"hello_world"};
Username sub = u.substr(0, 5);   // returns Username{"hello"}, not std::string
```

See the [forwarding tutorial](forwarding.md) for details.

### Interactions (Cross-Type Operators)

The operators above work on a **single type** (`Distance + Distance -> Distance`).
For operations **between different types**, use interactions:

```
# types.atlas
[struct commerce::Price]
description=double; +, -, <=>

[struct commerce::Quantity]
description=int; +, -, <=>

[struct commerce::Total]
description=double; +, -, <=>
```

```
# interactions.atlas
include "types.hpp"

namespace=commerce

Price * Quantity -> Total
Total / Price -> Quantity
Total / Quantity -> Price
Price * int <-> Price           # <-> means commutative (int * Price also works)
```

```cpp
Price unit_price{9.99};
Quantity qty{3};
Total total = unit_price * qty;   // Total{29.97}
Quantity back = total / unit_price; // Quantity{3}
```

Interactions support all arithmetic (`+`, `-`, `*`, `/`, `%`), bitwise (`&`, `|`, `^`, `<<`, `>>`), comparison (`==`, `!=`, `<`, `<=`, `>`, `>=`, `<=>`), and logical (`&&`, `||`) operators.
Compound assignment (`+=`, `-=`, etc.) is generated automatically when the result type matches the LHS type.

A `<=>` interaction gives you all six comparisons. C++20 rewrites `<`, `<=`, `>`,
and `>=` from `operator<=>`, but it only synthesizes `==` from a *defaulted*
`operator<=>` — and a cross-type one cannot be defaulted. Atlas therefore
generates `operator==` alongside it, from which C++20 rewrites `!=`:

```
# interactions.atlas
MaxRetries <=> int <-> auto
```

```cpp
MaxRetries retries{3};

retries < 5;    // rewritten from <=>
retries == 3;   // generated alongside <=>
retries != 4;   // rewritten from ==
```

Write an explicit `==` interaction for the same pair when you want different
semantics; yours replaces the generated one rather than colliding with it.

Additional interaction directives:
- **`value_access=`**: Controls how values are extracted from types (default: `atlas::undress`)
- **`lhs_value_access=`** / **`rhs_value_access=`**: Override value access per operand
- **`constexpr`** / **`no-constexpr`**: Control constexpr qualification per interaction
- **`concept=`** / **`enable_if=`**: Add template type constraints for generic interactions

Without interactions, you'd have to unwrap both types, multiply the raw values, and re-wrap.

### Direct Access Operators (`->`, `@`, `()`)

For types that wrap smart pointers, handles, or callable objects, these operators provide transparent access without unwrapping:

| Operator | Description | Syntax |
|----------|-------------|--------|
| `->` | Member access (like a pointer) | `->` |
| `@` | `operator*` dereference | `@` |
| `()` | Nullary call returning the value | `()` |
| `(&)` | Call operator forwarding to callable | `(&)` |
| `[]` | Subscript | `[]` |
| `&of` | `operator&` returning address of underlying | `&of` |

Example for a smart pointer wrapper:
```
[struct ResourceHandle]
description=std::shared_ptr<Resource>; ->, @, bool, no-constexpr
forward=get,reset
```

```cpp
ResourceHandle handle{std::make_shared<Resource>()};

handle->process();              // -> operator
Resource & res = *handle;       // @ (dereference) operator
if (handle) { ... }             // bool conversion
auto * raw = handle.get();      // forwarded member function
```

### Conversion Operators

```
[struct net::Port]
description=uint16_t; ==, <=>, cast<int>
```

```cpp
Port p{8080};
int raw = static_cast<int>(p);   // explicit cast<int>
```

Use `implicit_cast<Type>` for implicit conversions (rare; reduces type safety).
Use `bool` for explicit boolean conversion.

### Cross-Type Construction (`from_atlas_cast<A, B, ...>`)

When two *related* types share a compatible underlying type, `from_atlas_cast<...>` names the types you may construct from:

```
[struct net::TcpPort]
description=uint16_t; ==, <=>, from_atlas_cast<net::UdpPort>

[struct net::UdpPort]
description=uint16_t; ==, <=>, from_atlas_cast<net::TcpPort>
```

```cpp
TcpPort tcp{8080};
UdpPort udp{tcp};    // OK: TcpPort is listed
UdpPort bad{RetryCount{3}};  // Error: unlisted, even with the same underlying
```

For a family declared in one file, `auto_from_atlas_cast=true` at file level
allows cross-construction among the same-file types (and only those); an
explicit list unions with it. A bare `from_atlas_cast` token is a
generation-time error.

Value categories are preserved: lvalue sources copy, rvalue sources move.
If the constructed type's underlying *borrows* (a view-backed type), a
temporary argument would leave it dangling, so that construction is a
compile error — bind the source to a named variable first.

---

## When You Must Access the Raw Value

Sometimes you genuinely need the underlying value -- serialization, FFI calls, legacy interop.
Atlas provides four mechanisms, from lowest-level to most targeted.

### `atlas_value_for` (Hidden Friend)

This is the **primitive** that everything else is built on.
Each generated type has three hidden friend overloads found via ADL:

```cpp
int const & atlas_value_for(SimpleInt const & self);   // const lvalue -> const ref
int &       atlas_value_for(SimpleInt & self);          // mutable lvalue -> mutable ref
int         atlas_value_for(SimpleInt && self);          // rvalue -> by value (safe)
```

This removes exactly **one layer**, does **not** recurse, and is rarely called directly.
Prefer `unwrap`, `undress`, or `cast` instead.

### `atlas::unwrap` (One Layer)

Removes exactly **one layer** of wrapping. Works on atlas types and enums.

```cpp
NestedInt x{SimpleInt{42}};

atlas::unwrap(x);       // SimpleInt& -- removed one layer, stopped
atlas::unwrap(42);      // compile error -- int has no layer to remove
```

### `atlas::undress` (All Layers)

Recursively drills through **all layers** to the innermost raw type:

```cpp
TripleNestedInt x{NestedInt{SimpleInt{42}}};

atlas::undress(x);      // int& -- drilled through everything
atlas::undress(42);     // int& -- identity for non-atlas types
```

### `atlas::cast<T>` (Targeted Drilling)

Drills down until `static_cast<T>` succeeds. Stops at the first match:

```cpp
TripleNestedInt x{NestedInt{SimpleInt{42}}};

atlas::cast<int>(x);           // 42 -- found int at the bottom
atlas::cast<SimpleInt>(x);     // SimpleInt{42} -- stopped early
atlas::cast<double>(x);        // 42.0 -- found int, cast to double
```

#### Value targets vs reference targets

**Value targets** (`cast<int>`) always return by value, from any value
category. Rvalue sources move, layer by layer:
```cpp
auto val = atlas::cast<int>(std::move(x));   // int -- safe copy/move
```

**Reference targets** (`cast<int&>`, `cast<int const&>`, `cast<int&&>`)
require an **lvalue source** and bind to an actual layer of the chain:
```cpp
// Mutable reference -- can modify the wrapped value
int & ref = atlas::cast<int &>(x);
ref = 100;   // modifies x's innermost value

// Move semantics from a named object: use a value target
NestedInt n{SimpleInt{42}};
int moved = atlas::cast<int>(std::move(n));  // moves out, no reference held

// Reference target from an rvalue is a compile error: the reference
// would outlive the temporary it points into.
// atlas::cast<int &&>(NestedInt{SimpleInt{7}});         // error
// atlas::cast<std::string const &>(make_id());          // error
```

A reference target also never binds through a conversion: if reaching `T`
requires converting a layer's value (materializing a temporary `T`), the
cast is rejected rather than returning a reference to that temporary.

#### Borrowing targets and temporaries

When the target *borrows* storage a layer *owns* (`std::string` →
`std::string_view`), drilling a temporary would produce a dangling view, so
it is rejected at compile time even for value targets:

```cpp
Plain make_plain();   // std::string-backed, no conversion clauses declared

auto sv1 = atlas::cast<std::string_view>(make_plain());  // error: dangles
Plain p = make_plain();
auto sv2 = atlas::cast<std::string_view>(p);             // OK: views p
auto s   = atlas::cast<std::string>(make_plain());       // OK: owns (moves)
```

The classification behind this (`atlas::borrow_policy`) is user-extensible;
see [Rvalue Borrow Guards](description-language.md#rvalue-borrow-guards-lifetime-safety).

---

## Traits and Concepts

Generic code frequently needs to ask questions *about* a type rather than reach into a value.
Atlas ships these in namespace `atlas` -- there is no need to write your own `atlas_value_type` detection.

### Is it an atlas type?

```cpp
atlas::is_atlas_type<SimpleInt>::value;   // true
atlas::is_atlas_type<SimpleInt const &>::value;   // true -- cv and refs ignored
atlas::is_atlas_type<int>::value;         // false
atlas::is_atlas_type<Color>::value;       // false -- an enum is not an atlas type

template <atlas::AtlasTypeC T>
void log(T const & t) { ... }             // C++20 concept form
```

`atlas::holds_enum<T>` / `atlas::HoldsEnumC` answer the related question of whether `T` is,
or eventually wraps, an enum.

### What does it wrap?

`unwrap_t` and `undress_t` are the type-level counterparts of `unwrap` and `undress`.
Both ignore cv-qualification and references on the source type.

```cpp
atlas::unwrap_t<NestedInt>;        // SimpleInt -- one layer
atlas::undress_t<NestedInt>;       // int       -- all layers

atlas::undress_t<int>;             // int -- identity for non-atlas types
atlas::undress_t<WrappedColor>;    // int -- drills the enum to its underlying type
```

`unwrap_t` mirrors `unwrap`'s SFINAE behavior: it is a **substitution failure**, not a hard error,
when `T` is neither an atlas type nor an enum. `undress_t` is always well-formed.

### What kind of thing does it wrap?

These constrain on the **root** type, so they see through arbitrary nesting:

```cpp
atlas::AtlasOfC<T, U>          // T is an atlas type and undress_t<T> is exactly U
atlas::StringAtlasC<T>         // AtlasOfC<T, std::string>
atlas::ArithmeticAtlasC<T>     // root is arithmetic, excluding bool
```

Each is spelled as a conjunction including `AtlasTypeC`, so it **subsumes** it.
An overload constrained on `StringAtlasC` beats one constrained on `AtlasTypeC` instead of being ambiguous with it:

```cpp
template <atlas::AtlasTypeC T>    void emit(T const &);   // general
template <atlas::StringAtlasC T>  void emit(T const &);   // wins for string-rooted types
```

For pre-C++20 code, the equivalent traits are available unconditionally:
`atlas::is_atlas_of<T, U>`, `atlas::is_string_atlas<T>`, `atlas::is_arithmetic_atlas<T>`.

Note that `bool` is deliberately excluded from `ArithmeticAtlasC` even though `std::is_arithmetic_v<bool>` is `true`.
An enum-rooted type *is* arithmetic, because `undress_t` drills enums to their underlying type;
use `HoldsEnumC` if you need to exclude those.

---

## Return Type Reference

All non-template access utilities (`atlas_value_for`, `unwrap`, `undress`) return **by value** for rvalue sources.
This prevents dangling references from temporaries.

`atlas::cast<T>` with a **reference** target requires an lvalue source for the same reason: a reference into a temporary would dangle, so those casts are compile errors rather than lifetime hazards.

### `atlas_value_for`

| Source | Return type |
|--------|-------------|
| `T const &` | `underlying_type const &` |
| `T &` | `underlying_type &` |
| `T &&` | `underlying_type` (by value) |

### `atlas::unwrap`

| Source | Return type |
|--------|-------------|
| `SimpleInt const &` | `int const &` |
| `SimpleInt &` | `int &` |
| `SimpleInt &&` | `int` (by value) |
| `NestedInt const &` | `SimpleInt const &` |
| `NestedInt &` | `SimpleInt &` |
| `NestedInt &&` | `SimpleInt` (by value) |

### `atlas::undress`

| Source | Return type |
|--------|-------------|
| `SimpleInt const &` | `int const &` |
| `SimpleInt &` | `int &` |
| `SimpleInt &&` | `int` (by value) |
| `NestedInt const &` | `int const &` |
| `NestedInt &` | `int &` |
| `NestedInt &&` | `int` (by value) |

### `atlas::cast<T>`

| Target | Source | Return type |
|--------|--------|-------------|
| `int` (value) | `SimpleInt const &` | `int` |
| `int` (value) | `SimpleInt &&` | `int` (moves) |
| `int const &` | `SimpleInt const &` | `int const &` |
| `int &` | `SimpleInt &` | `int &` |
| `int &` | `NestedInt &` | `int &` (drills through) |
| `int &&` | `SimpleInt &` | `int &&` (explicit move from lvalue) |
| `int const &` | `SimpleInt &&` | compile error (would dangle) |
| `int &&` | `SimpleInt &&` | compile error (would dangle) |
| `std::string_view` | string-backed `&` | `std::string_view` (borrows lvalue) |
| `std::string_view` | string-backed `&&` | compile error (would dangle) |

---

## Best Practices

1. **Declare operators, don't unwrap for them.** If you need `<`, add `<=>` to the description. If you need `+`, add `+`. Don't reach into the wrapper.

2. **Use `forward` for member functions.** If you need `.size()` or `.empty()`, forward them. This gives a controlled API surface without exposing everything.

3. **Use interactions for cross-type math.** `Price * Quantity -> Total` is clearer and safer than unwrapping both operands.

4. **Use `auto_hash`, `auto_ostream`, `auto_format` instead of unwrapping for containers and I/O.** These are file-level directives that save unwrapping at every use site.

5. **Reserve `undress`/`cast` for system boundaries.** Serialization, FFI, legacy code, debug logging when `out` isn't declared.

6. **Prefer `cast<T>` over `undress` when you want a specific type.** `cast<SimpleInt>` stops at the right level; `undress` always goes to the bottom.

7. **Use reference targets on `cast` only when you need aliasing of a named object.** Reference targets require lvalue sources; to move out of an expiring object use a value target (`cast<int>(std::move(x))`).

8. **When a borrow guard rejects your call, bind a named variable.** `f(atlas::cast<std::string_view>(make_id()))` is rejected even though this one call would be safe, because the same expression pattern dangles everywhere else. Write `auto id = make_id();` and borrow from `id`.

9. **If you're unwrapping everywhere, your type description is incomplete.** Go back and add the missing operators, forwardings, or interactions.
