#C++ Standard Compatibility

Atlas Strong Type Generator is designed to work with C++11 and later standards, with automatic feature detection and fallbacks for maximum portability.

## Minimum Requirements

**C++11** is the minimum required standard for all generated code.

The generator ensures that:
- Type traits use C++11 syntax (`std::enable_if<...>::type`, `std::is_constructible<...>::value`)
- C++17 `void_t` is manually implemented for C++11 compatibility
- Feature test macros are used to conditionally enable modern C++ features

## Core Features (C++11 Compatible)

All basic strong type features work with C++11:

| Feature | Description | C++11 |
|---------|-------------|-------|
| Basic wrapper | Strong type wrapper with value member | ✅ |
| Copy/move semantics | Default copy/move constructors and assignment | ✅ |
| Variadic constructor | Perfect forwarding constructor with SFINAE | ✅ |
| Cast operators | Explicit conversion to underlying type | ✅ |
| Arithmetic operators | `+`, `-`, `*`, `/`, `%` with compound assignments | ✅ |
| Comparison operators | `==`, `!=`, `<`, `<=`, `>`, `>=` | ✅ |
| Logical operators | `&&`, `||`, `not` | ✅ |
| Bitwise operators | `&`, `|`, `^`, `~`, `<<`, `>>` | ✅ |
| Increment/decrement | `++`, `--` (prefix and postfix) | ✅ |
| Unary operators | `+`, `-` | ✅ |
| Stream operators | `<<`, `>>` for iostreams | ✅ |
| Hash support | `std::hash` specialization | ✅ |
| Addressof operators | `operator &()`, `operator ->()` | ✅ |
| Indirection operator | `operator *()` | ✅ |
| Bool conversion | `explicit operator bool()` | ✅ |
| Logical not | `operator not()` | ✅ |

## Constexpr Support

**User-controlled**: Whether operators are `constexpr` is determined by the user's type description, not by the C++ standard.

- **C++11**: Supports `constexpr` for simple operations (single return statement)
- **C++14+**: Supports `constexpr` for complex operations (multiple statements, assignments)
- **C++20+**: Supports `constexpr` destructors

If you specify `constexpr` in your type description and compile with C++11:
- ✅ Simple operations (getters, member const functions) will compile
- ❌ Complex operations (assignments, mutations) will fail to compile

**Recommendation**: If targeting C++11, only use `constexpr` for types with simple operations, or compile with C++14+.

## Optional Features with C++ Version Requirements

Some advanced features require modern C++ standards:

| Feature | C++ Version Required | Reason | Opt-in |
|---------|---------------------|--------|--------|
| **Subscript operator** (`[]`) | C++11 | Uses trailing return type with `decltype` | ✅ User opts-in |
| **Callable operator** (`()`) | C++11 | Direct call;

C++ 17 uses `std::invoke` | ✅ User opts - in | |
    **Spaceship operator ** (`<=>`) | C++ 20 | Language feature
    | ✅ User opts - in | | **Multidimensional subscript ** | C++ 23 |
    Multidimensional `operator[]` | Auto - detected |

    ## #Subscript Operator (C++ 11 +)

```cpp
    // C++11 compatible using trailing return type
    template <typename ArgT>
    constexpr auto
    operator[] (ArgT && arg)
    -> decltype(value[std::forward<ArgT>(arg)])
{
    return value[std::forward<ArgT>(arg)];
}

```

**How to use**: Add `[]` to your type description.

**C++ Version**: C++11 for single-argument subscript, C++23 for multidimensional subscript (automatically detected).

### Callable Operator (C++11+)

The callable operator uses feature detection to provide the best implementation for your C++ standard:

**C++17+ (with `std::invoke`):**
```cpp
template <typename InvocableT>
constexpr auto operator () (InvocableT && inv) const
-> decltype(std::invoke(std::forward<InvocableT>(inv), value))
{
    return std::invoke(std::forward<InvocableT>(inv), value);
}

```

        ** C++ 11 /
    14(direct call)
: **
```cpp template <typename InvocableT>
constexpr auto operator () (InvocableT && inv) const
-> decltype(std::forward<InvocableT>(inv)(value))
{
    return std::forward<InvocableT>(inv)(value);
}

```

**How to use**: Add `()` to your type description.

**C++11/14 Limitations**: Direct call works with function objects, lambdas, and function pointers, but not with member function pointers or member object pointers (use C++17+ with `std::invoke` for those).

### Spaceship Operator (C++20+)

``` constexpr cpp
// Requires C++20 for operator <=>
friend  auto operator <=> (
    ClassName const &,
    ClassName const &) = default;
```

        ** How to use **
: Add `<=>` to your type description
              .

                  ** Fallback **
: None.Compile with `-
      std = c++ 20` or
    later.

            * *Note *
            *
: When using `<=>`
, you typically don't need to specify individual comparison operators (`<`, `<=`, `>`, `>=`) as they are synthesized automatically.

    ## #Multidimensional Subscript(C++ 23 +)

```cpp
// Automatically uses C++23 feature when available
#if __cpp_multidimensional_subscript >= 202110L
    template <typename ArgT, typename... ArgTs>
    constexpr decltype(auto)
    operator[] (ArgT && arg, ArgTs &&... args)
{
    return value[std::forward<ArgT>(arg), std::forward<ArgTs>(args)...];
}
#else
    // Fallback to single-argument version
    template <typename ArgT>
    constexpr decltype(auto)
    operator[] (ArgT && arg)
{
    return value[std::forward<ArgT>(arg)];
}
#endif
```

**How to use**: Add `[]` to your type description. Multidimensional support is automatically detected.

**Fallback**: Single-argument subscript for C++14-C++20.

## Boilerplate Code Compatibility

The Atlas boilerplate (included in all generated files) uses feature detection for modern C++ features:

### Three-Way Comparison Support (C++20+)

```cpp
#if defined(__cpp_impl_three_way_comparison) && \
    __cpp_impl_three_way_comparison >= 201907L
    #include <compare>
#endif

struct strong_type_tag
{
#if defined(__cpp_impl_three_way_comparison) && \
    __cpp_impl_three_way_comparison >= 201907L
    friend auto operator <=> (
        strong_type_tag const &,
        strong_type_tag const &) = default;
#endif
};
```

    ## #Inline Variables(C++ 17 +)

``` inline constexpr cpp
#if defined(__cpp_inline_variables) && __cpp_inline_variables >= 201606L
    auto value = atlas_detail::Value{};
#else
    template <typename T>
    constexpr auto value(T && t)
    -> decltype(atlas_detail::Value{}(std::forward<T>(t)))
{
    return atlas_detail::Value{}(std::forward<T>(t));
}
#endif
```

**Fallback**: Function template for C++11-C++14.

### void_t (C++17 feature, provided for C++11)

```cpp
// Manual implementation for C++11 compatibility
template <typename... Ts>
struct make_void
{
    using type = void;
};

template <typename... Ts>
using void_t = typename make_void<Ts...>::type;
```

## Testing Your Generated Code

To verify compatibility with different C++ standards:

```bash
#Test with C++ 11
g++ -std=c++11 -fsyntax-only -Isrc/lib your_generated_file.hpp

#Test with C++ 14
g++ -std=c++14 -fsyntax-only -Isrc/lib your_generated_file.hpp

#Test with C++ 17
g++ -std=c++17 -fsyntax-only -Isrc/lib your_generated_file.hpp

#Test with C++ 20
g++ -std=c++20 -fsyntax-only -Isrc/lib your_generated_file.hpp
```

## CMake Integration

When using Atlas with CMake, set your minimum C++ standard:

```cmake
#For maximum compatibility
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#Or for modern features
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
```

## Summary

- ✅ **C++11**: All core features work, including subscript and callable operators
- ✅ **C++14**:  constexpr Enables  for more complex operations
- ✅ **C++17**: Adds `std::invoke` support for callable operator (member pointers), inline variables
- ✅ **C++20**: Adds spaceship operator, constexpr destructors
- ✅ **C++23**: Adds multidimensional subscript support

**Recommendation**: C++11 provides full functionality for nearly all use cases. Use C++17 or later for member pointer invocation and additional optimizations.
