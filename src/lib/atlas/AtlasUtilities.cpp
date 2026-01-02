// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "AtlasUtilities.hpp"

#include <boost/uuid/detail/sha1.hpp>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sstream>
#include <string>

#ifdef _WIN32
    #include <io.h>
    #define isatty _isatty
#else
    #include <unistd.h>
#endif

namespace wjh::atlas {

// ============================================================================
// String parsing utilities
// ============================================================================

// Default predicate for strip - checks if character is whitespace
struct IsSpacePred
{
    bool operator () (unsigned char u) const
    {
        return std::isspace(static_cast<int>(u));
    }
};

template <typename PredT = IsSpacePred>
std::string_view
strip(std::string_view sv, PredT pred = PredT{})
{
    auto result = sv;
    while (not result.empty() &&
           pred(static_cast<unsigned char>(result.front())))
    {
        result.remove_prefix(1);
    }
    while (not result.empty() &&
           pred(static_cast<unsigned char>(result.back())))
    {
        result.remove_suffix(1);
    }
    return result;
}

std::vector<std::string_view>
split(std::string_view sv, char sep)
{
    std::vector<std::string_view> components;
    while (not sv.empty()) {
        while (not sv.empty() && std::isspace(sv.front())) {
            sv.remove_prefix(1);
        }
        // Find the next separator, respecting angle bracket nesting
        // Only count brackets when they're part of identifiers (bounded<0,100>)
        // not standalone comparison operators (like <, <=, >, >=)
        int bracket_depth = 0;
        size_t n = 0;
        while (n < sv.size()) {
            char c = sv[n];
            if (c == '<' && n > 0 &&
                std::isalnum(static_cast<unsigned char>(sv[n - 1])))
            {
                // This looks like a template parameter (e.g., bounded<...)
                // Only increase depth if preceded directly by alphanumeric (no
                // space)
                ++bracket_depth;
            } else if (c == '>' && bracket_depth > 0) {
                --bracket_depth;
            } else if (c == sep && bracket_depth == 0) {
                break;
            }
            ++n;
        }
        components.push_back(strip(sv.substr(0, n)));
        sv.remove_prefix(std::min(n + 1, sv.size()));
    }
    return components;
}

// ============================================================================
// ParsedSpecification
// ============================================================================

void
ParsedSpecification::
merge(ParsedSpecification const & other)
{
    // first_part from 'this' takes precedence (descriptions override profiles)
    // Only update if our first_part is empty
    if (first_part.empty()) {
        first_part = other.first_part;
    }

    // Merge forwards (append, preserving order)
    forwards.insert(
        forwards.end(),
        other.forwards.begin(),
        other.forwards.end());

    // Merge operators (union)
    operators.insert(other.operators.begin(), other.operators.end());
}

ParsedSpecification
parse_specification(std::string_view spec)
{
    ParsedSpecification result;

    auto segments = split(spec, ';');
    if (segments.empty()) {
        throw std::invalid_argument("Empty specification");
    }

    // First segment is always the "first part" (type or profile name)
    result.first_part = std::string(strip(segments[0]));

    // Remove "strong" prefix if present (for descriptions) but remember we had
    // it
    if (result.first_part.starts_with("strong ")) {
        result.had_strong_keyword = true;
        result.first_part = std::string(
            strip(std::string_view(result.first_part).substr(7)));
    }

    if (result.first_part.empty()) {
        throw std::invalid_argument("Empty type specification in description");
    }

    // Process remaining segments
    for (size_t i = 1; i < segments.size(); ++i) {
        auto segment = strip(segments[i]);

        if (segment.empty()) {
            continue; // Skip empty segments
        }

        // Check if it's a forward= specification
        if (segment.starts_with("forward=")) {
            auto memfn_str = segment.substr(8);
            if (memfn_str.empty()) {
                throw std::invalid_argument(
                    "Empty forward= specification (forward= must be followed "
                    "by member function names)");
            }

            // Split by comma and add to forwards vector (preserving order!)
            for (auto memfn : split(memfn_str, ',')) {
                auto trimmed = std::string(strip(memfn));
                if (not trimmed.empty()) {
                    result.forwards.push_back(trimmed);
                }
            }
        } else {
            // It's an operators segment - split by comma
            for (auto op : split(segment, ',')) {
                auto trimmed_view = strip(op);
                // MUST create a std::string copy because op is a string_view
                // that will be invalidated when we return from this function
                std::string trimmed_str(trimmed_view);
                if (not trimmed_str.empty()) {
                    result.operators.insert(std::move(trimmed_str));
                }
            }
        }
    }

    return result;
}

// ============================================================================
// End of string parsing utilities
// ============================================================================

// Note: get_sha1 has been moved to generation/core/SHA1Hasher.cpp
// to avoid circular dependency issues between atlas_lib and generation library

std::string
generate_header_guard(
    std::string const & prefix,
    std::string const & separator,
    std::string const & content_hash,
    bool upcase)
{
    std::string guard_prefix = prefix.empty() ? "ATLAS" : prefix;
    std::string guard = guard_prefix + separator + content_hash;

    if (upcase) {
        std::transform(
            guard.begin(),
            guard.end(),
            guard.begin(),
            [](unsigned char c) { return std::toupper(c); });
    }

    return guard;
}

std::string
trim(std::string const & str)
{
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

std::vector<std::string>
get_preamble_includes(PreambleOptions const & options)
{
    std::vector<std::string> includes;

    // Base includes always needed by preamble
    includes.push_back("<type_traits>");
    includes.push_back("<utility>");

    if (options.include_constraints) {
        includes.push_back("<sstream>");
        includes.push_back("<stdexcept>");
        includes.push_back("<string>");
    }

    if (options.include_nilable_support) {
        includes.push_back("<cassert>");
        includes.push_back("<functional>");
        includes.push_back("<memory>");
        includes.push_back("<optional>");
        includes.push_back("<type_traits>");
        includes.push_back("<utility>");
    }

    return includes;
}

std::string
preamble(PreambleOptions options)
{
    static constexpr char const basic[] = R"(
#ifndef WJH_ATLAS_50E620B544874CB8BE4412EE6773BF90
#define WJH_ATLAS_50E620B544874CB8BE4412EE6773BF90

// ======================================================================
// ATLAS STRONG TYPE BOILERPLATE
// ----------------------------------------------------------------------
//
// This section provides the infrastructure for Atlas strong types.
// It is identical across all Atlas-generated files and uses a shared
// header guard (WJH_ATLAS_50E620B544874CB8BE4412EE6773BF90) to ensure
// the boilerplate is only included once even when multiple generated
// files are used in the same translation unit.
//
// The boilerplate is intentionally inlined to make generated code
// self-contained with zero external dependencies.
//
// Components:
// - atlas::strong_type_tag: Base class for strong types
// - atlas::to_underlying(): Universal value accessor for strong types
// - atlas_detail::*: Internal implementation utilities
//
// For projects using multiple Atlas-generated files, this boilerplate
// will only be compiled once per translation unit thanks to the shared
// header guard below.
//
// ----------------------------------------------------------------------
// DO NOT EDIT THIS SECTION
// ======================================================================

// Atlas feature detection macros
#ifndef ATLAS_NODISCARD
#if defined(__cpp_attributes) && __cpp_attributes >= 201603L
#define ATLAS_NODISCARD [[nodiscard]]
#else
#define ATLAS_NODISCARD
#endif
#endif

#if defined(__cpp_impl_three_way_comparison) && \
    __cpp_impl_three_way_comparison >= 201907L
#include <compare>
#endif

namespace atlas {

struct strong_type_tag
{
#if defined(__cpp_impl_three_way_comparison) && \
    __cpp_impl_three_way_comparison >= 201907L
    friend auto operator <=> (
        strong_type_tag const &,
        strong_type_tag const &) = default;
#endif
};

struct value_tag
{ };

namespace atlas_detail {

template <typename... Ts>
struct make_void
{
    using type = void;
};

template <typename... Ts>
using void_t = typename make_void<Ts...>::type;

template <std::size_t N>
struct PriorityTag
: PriorityTag<N - 1>
{ };

template <>
struct PriorityTag<0u>
{ };

using value_tag = PriorityTag<3>;

template <bool B>
using bool_c = std::integral_constant<bool, B>;
template <typename T>
using bool_ = bool_c<T::value>;
template <typename T>
using not_ = bool_c<not T::value>;
template <typename T, typename U>
using and_ = bool_c<T::value && U::value>;
template <typename T>
using is_lref = std::is_lvalue_reference<T>;

template <typename T>
using remove_cv_t = typename std::remove_cv<T>::type;
template <typename T>
using remove_reference_t = typename std::remove_reference<T>::type;
template <typename T>
using remove_cvref_t = remove_cv_t<remove_reference_t<T>>;
template <bool B, typename T = void>
using enable_if_t = typename std::enable_if<B, T>::type;
template <bool B>
using when = enable_if_t<B, bool>;

template <typename T>
using _t = typename T::type;

template <typename T, typename = void>
struct has_atlas_value_type
: std::false_type
{ };

template <typename T>
struct has_atlas_value_type<
    T,
    enable_if_t<not std::is_same<
        typename remove_cvref_t<T>::atlas_value_type,
        void>::value>>
: std::true_type
{ };

void atlas_value_for();
struct value_by_ref
{ };
struct value_by_val
{ };

// ----------------------------------------------------------------------------
// Base case: T does not have atlas_value_type
// These are the termination cases for the recursion.
// ----------------------------------------------------------------------------
template <typename T>
constexpr T &
value_impl(T & t, PriorityTag<0>, value_by_ref)
{
    return t;
}
template <typename T>
constexpr T const &
value_impl(T const & t, PriorityTag<0>, value_by_ref)
{
    return t;
}
template <typename T>
constexpr T
value_impl(T & t, PriorityTag<0>, value_by_val)
{
    return std::move(t);
}
template <typename T>
constexpr T
value_impl(T const & t, PriorityTag<0>, value_by_val)
{
    return t;
}

// ----------------------------------------------------------------------------
// Recursive case: T has atlas_value_for() hidden friend
// Use ADL to call atlas_value_for() and recurse.
// ----------------------------------------------------------------------------
template <typename T>
constexpr auto
value_impl(T & t, PriorityTag<1>, value_by_ref)
-> decltype(value_impl(
    atlas_value_for(t),
    value_tag{},
    value_by_ref{}))
{
    return value_impl(atlas_value_for(t), value_tag{}, value_by_ref{});
}
template <typename T>
constexpr auto
value_impl(T const & t, PriorityTag<1>, value_by_ref)
-> decltype(value_impl(
    atlas_value_for(t),
    value_tag{},
    value_by_ref{}))
{
    return value_impl(atlas_value_for(t), value_tag{}, value_by_ref{});
}
template <typename T>
constexpr auto
value_impl(T & t, PriorityTag<1>, value_by_val)
-> decltype(value_impl(
    atlas_value_for(std::move(t)),
    value_tag{},
    value_by_val{}))
{
    return value_impl(atlas_value_for(std::move(t)), value_tag{}, value_by_val{});
}
template <typename T>
constexpr auto
value_impl(T const & t, PriorityTag<1>, value_by_val)
-> decltype(value_impl(
    atlas_value_for(t),
    value_tag{},
    value_by_val{}))
{
    return value_impl(atlas_value_for(t), value_tag{}, value_by_val{});
}

struct ToUnderlying
{
    template <typename T>
    constexpr auto
    operator () (T & t) const
    -> decltype(atlas_detail::value_impl(t, value_tag{}, value_by_ref{}))
    {
        return atlas_detail::value_impl(t, value_tag{}, value_by_ref{});
    }

    template <typename T>
    constexpr auto
    operator () (T const & t) const
    -> decltype(atlas_detail::value_impl(t, value_tag{}, value_by_ref{}))
    {
        return atlas_detail::value_impl(t, value_tag{}, value_by_ref{});
    }

    template <
        typename T,
        when<not std::is_lvalue_reference<T>::value> = true>
    constexpr auto
    operator () (T && t) const
    -> decltype(atlas_detail::value_impl(t, value_tag{}, value_by_val{}))
    {
        return atlas_detail::value_impl(t, value_tag{}, value_by_val{});
    }
};

void begin();
void end();

template <typename T>
constexpr auto
begin_(T && t) noexcept(noexcept(begin(std::forward<T>(t))))
-> decltype(begin(std::forward<T>(t)))
{
    return begin(std::forward<T>(t));
}

template <typename T>
constexpr auto
end_(T && t) noexcept(noexcept(end(std::forward<T>(t))))
-> decltype(end(std::forward<T>(t)))
{
    return end(std::forward<T>(t));
}

} // namespace atlas_detail

using atlas_detail::enable_if_t;
using atlas_detail::remove_cv_t;
using atlas_detail::remove_cvref_t;
using atlas_detail::when;

template <typename T>
using is_atlas_type = atlas_detail::has_atlas_value_type<T>;

#if defined(__cpp_concepts) && __cpp_concepts >= 201907L
template <typename T>
concept AtlasTypeC = is_atlas_type<T>::value;
#endif

#if defined(__cpp_inline_variables) && __cpp_inline_variables >= 201606L
inline constexpr auto to_underlying = atlas_detail::ToUnderlying{};
#else
template <typename T>
constexpr auto
to_underlying(T && t)
-> decltype(atlas_detail::ToUnderlying{}(std::forward<T>(t)))
{
    return atlas_detail::ToUnderlying{}(std::forward<T>(t));
}
#endif

} // namespace atlas

#endif // WJH_ATLAS_50E620B544874CB8BE4412EE6773BF90
)";

    static constexpr char const const_mutable[] = R"(
#ifndef WJH_ATLAS_46CE143CD5E7495DAA505B54DBD417A2
#define WJH_ATLAS_46CE143CD5E7495DAA505B54DBD417A2

namespace atlas {
namespace atlas_detail {

struct const_
{
    template <typename T>
    static T const * _ (T * p) { return p; }
    template <typename T>
    static T const & _ (T const & p) { return p; }
};

struct mutable_
{
    template <typename T>
    static T * _ (T * p) { return p; }
    template <typename T>
    static T && _ (T && p) { return static_cast<T&&>(p); }
};

} // namespace atlas_detail
} // namespace atlas

#endif // WJH_ATLAS_46CE143CD5E7495DAA505B54DBD417A2
)";

    static constexpr char const arrow_helpers[] = R"(
#ifndef WJH_ATLAS_A527B9864606413FB036AFD74BF8C8BF
#define WJH_ATLAS_A527B9864606413FB036AFD74BF8C8BF

namespace atlas {
namespace atlas_detail {

template <typename T, typename U>
auto
arrow_impl(U & u, PriorityTag<1>)
-> decltype(T::_(u.operator->()))
{
    return T::_(u.operator->());
}

template <typename T, typename U>
auto
arrow_impl(U * u, PriorityTag<1>)
-> decltype(T::_(u))
{
    return T::_(u);
}

template <typename T, typename U>
U * arrow_impl(U & u, PriorityTag<0>)
{
    return std::addressof(u);
}

} // namespace atlas_detail
} // namespace atlas

#endif // WJH_ATLAS_A527B9864606413FB036AFD74BF8C8BF
)";

    static constexpr char const star_helpers[] = R"(
#ifndef WJH_ATLAS_05F39F486A854621A7A80EA8B40E7665
#define WJH_ATLAS_05F39F486A854621A7A80EA8B40E7665

namespace atlas {
namespace atlas_detail {

template <typename T, typename U>
auto
star_impl(U & u, PriorityTag<1>)
-> decltype(T::_(u.operator*()))
{
    return T::_(u.operator*());
}

template <typename T, typename U>
auto
star_impl(U * u, PriorityTag<1>)
-> decltype(*T::_(u))
{
    return *T::_(u);
}

template <typename T, typename U>
U & star_impl(U & u, PriorityTag<0>)
{
    return u;
}

} // namespace atlas_detail
} // namespace atlas

#endif // WJH_ATLAS_05F39F486A854621A7A80EA8B40E7665
)";

    static constexpr char const checked_helpers[] = R"(
#ifndef WJH_ATLAS_8BF8485B2F9D45ACAD473DC5B3274DDF
#define WJH_ATLAS_8BF8485B2F9D45ACAD473DC5B3274DDF

#ifdef __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wweak-vtables"
#endif

namespace atlas {

/**
 * Base class for arithmetic-related errors in checked arithmetic mode.
 *
 * This serves as the base class for all arithmetic exceptions thrown by
 * checked arithmetic operations on Atlas strong types.
 *
 * @see CheckedOverflowError
 * @see CheckedUnderflowError
 * @see CheckedDivisionByZeroError
 * @see CheckedInvalidOperationError
 */
class ArithmeticError
: public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

/**
 * Thrown when an arithmetic operation goes above the maximum representable
 * value.
 *
 * This exception is thrown by checked arithmetic operations when the result
 * would exceed std::numeric_limits<T>::max() for the underlying type.
 *
 * @note This is distinct from std::overflow_error, which represents
 * floating-point overflow. This exception represents integer and
 * floating-point range violations in checked arithmetic operations.
 *
 * Examples:
 * - CheckedInt8{127} + CheckedInt8{1}
 * - CheckedInt{INT_MAX} + CheckedInt{1}
 * - CheckedDouble{DBL_MAX} + CheckedDouble{DBL_MAX}
 *
 * @see CheckedUnderflowError for negative range violations
 * @see std::overflow_error (different semantics!)
 */
class CheckedOverflowError
: public ArithmeticError
{
public:
    using ArithmeticError::ArithmeticError;
};

/**
 * Thrown when an arithmetic operation goes below the minimum representable
 * value.
 *
 * This exception is thrown by checked arithmetic operations when the result
 * would be less than std::numeric_limits<T>::min() for signed types, or less
 * than zero for unsigned types during subtraction.
 *
 * @note This is NOT the same as std::underflow_error, which represents
 * floating-point gradual underflow. This represents integer and
 * floating-point range violations in checked arithmetic operations.
 *
 * Examples:
 * - CheckedInt8{-128} - CheckedInt8{1}
 * - CheckedInt{INT_MIN} - CheckedInt{1}
 * - CheckedUInt{0} - CheckedUInt{1}
 *
 * @see CheckedOverflowError for positive range violations
 * @see std::underflow_error (different semantics!)
 */
class CheckedUnderflowError
: public ArithmeticError
{
public:
    using ArithmeticError::ArithmeticError;
};

/**
 * Thrown when dividing or taking modulo by zero in checked arithmetic mode.
 *
 * This exception is thrown by checked arithmetic operations when attempting
 * to divide or compute modulo with a zero divisor, which is undefined behavior
 * in C++.
 *
 * Examples:
 * - CheckedInt{5} / CheckedInt{0}
 * - CheckedInt{10} % CheckedInt{0}
 * - CheckedDouble{3.14} / CheckedDouble{0.0}
 *
 * @see CheckedInvalidOperationError for NaN-producing operations
 */
class CheckedDivisionByZeroError
: public ArithmeticError
{
public:
    using ArithmeticError::ArithmeticError;
};

/**
 * Thrown when a floating-point operation produces an invalid result (NaN).
 *
 * This exception is thrown by checked arithmetic operations when a floating-point
 * operation would produce NaN (Not-a-Number) according to IEEE 754 semantics.
 *
 * @note This is distinct from IEEE 754 invalid operation exceptions and
 * represents NaN detection in checked arithmetic mode, not hardware exception
 * handling.
 *
 * Examples:
 * - CheckedDouble{0.0} / CheckedDouble{0.0}  // 0/0 -> NaN
 * - CheckedDouble{INFINITY} - CheckedDouble{INFINITY}  // inf-inf -> NaN
 * - CheckedDouble{-1.0}.sqrt()  // sqrt(-1) -> NaN (if sqrt method exists)
 *
 * @see CheckedDivisionByZeroError for division by zero
 * @see CheckedOverflowError for overflow to infinity
 */
class CheckedInvalidOperationError
: public ArithmeticError
{
public:
    using ArithmeticError::ArithmeticError;
};

namespace atlas_detail {

template <typename T>
using EnableFloatingPoint = typename std::enable_if<
    std::is_floating_point<T>::value,
    T>::type;

template <typename T>
using EnableSigned = typename std::enable_if<
    std::is_signed<T>::value && std::is_integral<T>::value,
    T>::type;

template <typename T>
using EnableUnsigned = typename std::enable_if<
    std::is_unsigned<T>::value && std::is_integral<T>::value,
    T>::type;

template <typename T>
EnableFloatingPoint<T>
checked_add(T a, T b, char const * overflow, char const * underflow)
{
    T result = a + b;
    if (std::isinf(result)) {
        if (result > 0) {
            throw CheckedOverflowError(overflow);
        } else {
            throw CheckedUnderflowError(underflow);
        }
    }
    if (std::isnan(result)) {
        throw CheckedInvalidOperationError("Invalid operation: NaN result");
    }
    return result;
}

template <typename T>
EnableUnsigned<T>
checked_add(T a, T b, char const * error_msg, char const * = "")
{
#if defined(__GNUC__) || defined(__clang__)
    T result;
    if (__builtin_add_overflow(a, b, &result)) {
        throw CheckedOverflowError(error_msg);
    }
    return result;
#else
    if (a > std::numeric_limits<T>::max() - b) {
        throw CheckedOverflowError(error_msg);
    }
    return a + b;
#endif
}

template <typename T>
EnableSigned<T>
checked_add(T a, T b, char const * overflow, char const * underflow)
{
#if defined(__GNUC__) || defined(__clang__)
    T result;
    if (__builtin_add_overflow(a, b, &result)) {
        if (b < 0) {
            throw CheckedUnderflowError(underflow);
        } else {
            throw CheckedOverflowError(overflow);
        }
    }
    return result;
#else
    if (b > 0 && a > std::numeric_limits<T>::max() - b) {
        throw CheckedOverflowError(overflow);
    } else if (b < 0 && a < std::numeric_limits<T>::lowest() - b) {
        throw CheckedUnderflowError(underflow);
    }
    return a + b;
#endif
}

template <typename T>
EnableFloatingPoint<T>
checked_sub(T a, T b, char const * overflow, char const * underflow)
{
    a -= b;
    if (std::isinf(a)) {
        if (a > 0) {
            throw CheckedOverflowError(overflow);
        } else {
            throw CheckedUnderflowError(underflow);
        }
    } else if (std::isnan(a)) {
        throw CheckedInvalidOperationError("Invalid operation: NaN result");
    }
    return a;
}

template <typename T>
EnableUnsigned<T>
checked_sub(T a, T b, char const *, char const * underflow)
{
#if defined(__GNUC__) || defined(__clang__)
    T result;
    if (__builtin_sub_overflow(a, b, &result)) {
        throw CheckedUnderflowError(underflow);
    }
    return result;
#else
    if (a < b) {
        throw CheckedUnderflowError(underflow);
    }
    return a - b;
#endif
}

template <typename T>
EnableSigned<T>
checked_sub(T a, T b, char const * overflow, char const * underflow)
{
#if defined(__GNUC__) || defined(__clang__)
    T result;
    if (__builtin_sub_overflow(a, b, &result)) {
        if (b > 0) {
            throw CheckedUnderflowError(underflow);
        } else {
            throw CheckedOverflowError(overflow);
        }
    }
    return result;
#else
    if (b < 0 && a > std::numeric_limits<T>::max() + b) {
        throw CheckedOverflowError(overflow);
    } else if (b > 0 && a < std::numeric_limits<T>::lowest() + b) {
        throw CheckedUnderflowError(underflow);
    }
    return a - b;
#endif
}

template <typename T>
EnableFloatingPoint<T>
checked_mul(T a, T b, char const * overflow, char const *)
{
    // Check for multiplication that would produce NaN (inf * 0 or 0 * inf)
    if ((std::isinf(a) && b == static_cast<T>(0.0)) ||
        (a == static_cast<T>(0.0) && std::isinf(b))) {
        throw CheckedInvalidOperationError(overflow);
    }

    a *= b;
    if (std::isinf(a)) {
        throw CheckedOverflowError(overflow);
    } else if (std::isnan(a)) {
        throw CheckedInvalidOperationError(overflow);
    }
    return a;
}

template <typename T>
EnableUnsigned<T>
checked_mul(T a, T b, char const * overflow, char const *)
{
#if defined(__GNUC__) || defined(__clang__)
    T result;
    if (__builtin_mul_overflow(a, b, &result)) {
        throw CheckedOverflowError(overflow);
    }
    return result;
#else
    if (b != 0 && a > std::numeric_limits<T>::max() / b) {
        throw CheckedOverflowError(overflow);
    }
    return a * b;
#endif
}

template <typename T>
EnableSigned<T>
checked_mul(T a, T b, char const * overflow, char const * underflow)
{
#if defined(__GNUC__) || defined(__clang__)
    T result;
    if (__builtin_mul_overflow(a, b, &result)) {
        // Determine if overflow or underflow based on operand signs
        bool same_sign = (a > 0) == (b > 0);
        if (same_sign) {
            throw CheckedOverflowError(overflow);
        } else {
            throw CheckedUnderflowError(underflow);
        }
    }
    return result;
#else
    // Handle zero cases
    if (a == 0 || b == 0) {
        return 0;
    }

    // Check for __int128 support (GCC/Clang on 64-bit platforms)
#if defined(__SIZEOF_INT128__) && (sizeof(T) < 16)
    // Use __int128 for widening (works for all types up to 64-bit)
    __int128 result = static_cast<__int128>(a) * static_cast<__int128>(b);
    if (result < static_cast<__int128>(std::numeric_limits<T>::lowest()) ||
        result > static_cast<__int128>(std::numeric_limits<T>::max()))
    {
        throw CheckedOverflowError(overflow);
    }
    return static_cast<T>(result);
#else
    // Fallback: widening for small types, division checks for long long
    if (sizeof(T) < sizeof(long long)) {
        auto result = static_cast<long long>(a) * static_cast<long long>(b);
        if (result < static_cast<long long>(std::numeric_limits<T>::lowest()) ||
            result > static_cast<long long>(std::numeric_limits<T>::max()))
        {
            throw CheckedOverflowError(overflow);
        }
        return static_cast<T>(result);
    } else {
        // For long long itself (or __int128 if that's T), use division checks
        // Check all four sign combinations
        if (a > 0) {
            if (b > 0) {
                if (a > std::numeric_limits<T>::max() / b) {
                    throw CheckedOverflowError(overflow);
                }
            } else {
                if (b < std::numeric_limits<T>::lowest() / a) {
                    throw CheckedOverflowError(overflow);
                }
            }
        } else {
            if (b > 0) {
                if (a < std::numeric_limits<T>::lowest() / b) {
                    throw CheckedOverflowError(overflow);
                }
            } else {
                if (a != 0 && b < std::numeric_limits<T>::max() / a) {
                    throw CheckedOverflowError(overflow);
                }
            }
        }
        return a * b;
    }
#endif
#endif
}

template <typename T>
EnableFloatingPoint<T>
checked_div(T a, T b, char const * div_by_zero, char const *)
{
    // Division by zero: throw exception (including 0.0/0.0 which produces NaN)
    if (b == T(0)) {
        throw CheckedDivisionByZeroError(div_by_zero);
    }
    // Check for inf / inf which produces NaN
    if (std::isinf(a) && std::isinf(b)) {
        throw CheckedInvalidOperationError(div_by_zero);
    }
    a /= b;
    if (std::isinf(a)) {
        throw CheckedOverflowError(div_by_zero);
    } else if (std::isnan(a)) {
        throw CheckedInvalidOperationError(div_by_zero);
    }
    return a;
}

template <typename T>
EnableUnsigned<T>
checked_div(T a, T b, char const * div_by_zero, char const *)
{
    if (b == T(0)) {
        throw CheckedDivisionByZeroError(div_by_zero);
    }
    return a / b;
}

template <typename T>
EnableSigned<T>
checked_div(T a, T b, char const * div_by_zero, char const * overflow)
{
    if (b == T(0)) {
        throw CheckedDivisionByZeroError(div_by_zero);
    }
    // Check for special case: INT_MIN / -1 overflows
    if (a == std::numeric_limits<T>::lowest() && b == T(-1)) {
        throw CheckedOverflowError(overflow);
    }
    return a / b;
}

template <typename T>
typename std::enable_if<std::is_integral<T>::value, T>::type
checked_mod(T a, T b, char const * div_by_zero)
{
    if (b == T(0)) {
        throw CheckedDivisionByZeroError(div_by_zero);
    }
    // INT_MIN % -1 is UB - throw for consistency with INT_MIN / -1
    if (std::is_signed<T>::value &&
        a == std::numeric_limits<T>::lowest() &&
        b == static_cast<T>(-1)) {
        throw CheckedOverflowError(div_by_zero);  // Consistent with division
    }
    return a % b;
}

// Modulo for floating-point - not provided (use static_assert in caller)

} // namespace atlas_detail
} // namespace atlas

#ifdef __clang__
    #pragma clang diagnostic pop
#endif

#endif // WJH_ATLAS_8BF8485B2F9D45ACAD473DC5B3274DDF
)";

    static constexpr char const saturating_helpers[] = R"(
#ifndef WJH_ATLAS_64A9A0E1C2564622BBEAE087A98B793D
#define WJH_ATLAS_64A9A0E1C2564622BBEAE087A98B793D

namespace atlas {
namespace atlas_detail {

template <typename T>
using EnableFloatingPoint = typename std::enable_if<
    std::is_floating_point<T>::value,
    T>::type;

template <typename T>
using EnableSigned = typename std::enable_if<
    std::is_signed<T>::value && std::is_integral<T>::value,
    T>::type;

template <typename T>
using EnableUnsigned = typename std::enable_if<
    std::is_unsigned<T>::value && std::is_integral<T>::value,
    T>::type;

template <typename T>
EnableFloatingPoint<T>
saturating_add(T a, T b) noexcept
{
    T result = a + b;
    if (std::isinf(result) || result > std::numeric_limits<T>::max()) {
        return std::numeric_limits<T>::max();
    }
    if (result < std::numeric_limits<T>::lowest()) {
        return std::numeric_limits<T>::lowest();
    }
    // Saturate NaN to max for consistency
    if (std::isnan(result)) {
        return std::numeric_limits<T>::max();
    }
    return result;
}

template <typename T>
EnableUnsigned<T>
saturating_add(T a, T b) noexcept
{
    if (a > std::numeric_limits<T>::max() - b) {
        return std::numeric_limits<T>::max();
    }
    return a + b;
}

template <typename T>
EnableSigned<T>
saturating_add(T a, T b) noexcept
{
    if (b > 0 && a > std::numeric_limits<T>::max() - b) {
        return std::numeric_limits<T>::max();
    }
    if (b < 0 && a < std::numeric_limits<T>::lowest() - b) {
        return std::numeric_limits<T>::lowest();
    }
    return a + b;
}

template <typename T>
EnableFloatingPoint<T>
saturating_sub(T a, T b) noexcept
{
    T result = a - b;
    if (std::isinf(result) || result > std::numeric_limits<T>::max()) {
        return std::numeric_limits<T>::max();
    }
    if (result < std::numeric_limits<T>::lowest()) {
        return std::numeric_limits<T>::lowest();
    }
    // Saturate NaN to max for consistency
    if (std::isnan(result)) {
        return std::numeric_limits<T>::max();
    }
    return result;
}

template <typename T>
EnableUnsigned<T>
saturating_sub(T a, T b) noexcept
{
    if (a < b) {
        return std::numeric_limits<T>::min(); // 0 for unsigned
    }
    return a - b;
}

template <typename T>
EnableSigned<T>
saturating_sub(T a, T b) noexcept
{
    if (b < 0 && a > std::numeric_limits<T>::max() + b) {
        return std::numeric_limits<T>::max();
    }
    if (b > 0 && a < std::numeric_limits<T>::lowest() + b) {
        return std::numeric_limits<T>::lowest();
    }
    return a - b;
}

/**
 * Saturating multiplication for floating-point types
 *
 * Multiplies two floating-point values and clamps the result to the
 * representable range if overflow or underflow occurs.
 *
 * @tparam T Floating-point type (float, double, long double)
 * @param a First operand
 * @param b Second operand
 * @return Product of a and b, clamped to [lowest, max]
 */
template <typename T>
EnableFloatingPoint<T>
saturating_mul(T a, T b) noexcept
{
    T result = a * b;
    if (std::isinf(result) || result > std::numeric_limits<T>::max()) {
        return std::numeric_limits<T>::max();
    }
    if (result < std::numeric_limits<T>::lowest()) {
        return std::numeric_limits<T>::lowest();
    }
    // NaN case: saturate to max for consistency
    if (std::isnan(result)) {
        return std::numeric_limits<T>::max();
    }
    return result;
}

/**
 * Saturating multiplication for unsigned integer types
 *
 * Multiplies two unsigned integers and clamps the result to the maximum
 * representable value if overflow occurs.
 *
 * @tparam T Unsigned integer type
 * @param a First operand
 * @param b Second operand
 * @return Product of a and b, clamped to max on overflow
 */
template <typename T>
EnableUnsigned<T>
saturating_mul(T a, T b) noexcept
{
#if defined(__GNUC__) || defined(__clang__)
    T result;
    if (__builtin_mul_overflow(a, b, &result)) {
        return std::numeric_limits<T>::max();
    }
    return result;
#else
    if (b != 0 && a > std::numeric_limits<T>::max() / b) {
        return std::numeric_limits<T>::max();
    }
    return a * b;
#endif
}

/**
 * Saturating multiplication for signed integer types
 *
 * Multiplies two signed integers and clamps the result to the representable
 * range if overflow or underflow occurs.
 *
 * @tparam T Signed integer type
 * @param a First operand
 * @param b Second operand
 * @return Product of a and b, clamped to [min, max]
 */
template <typename T>
EnableSigned<T>
saturating_mul(T a, T b) noexcept
{
#if defined(__GNUC__) || defined(__clang__)
    T result;
    if (__builtin_mul_overflow(a, b, &result)) {
        // Determine if overflow or underflow based on signs
        bool const same_sign = (a > 0) == (b > 0);
        if (same_sign) {
            return std::numeric_limits<T>::max();
        } else {
            return std::numeric_limits<T>::lowest();
        }
    }
    return result;
#else
    // Handle zero cases
    if (a == 0 || b == 0) {
        return 0;
    }

    // Check for __int128 support (GCC/Clang on 64-bit platforms)
#if defined(__SIZEOF_INT128__) && (sizeof(T) < 16)
    // Use __int128 for widening (works for all types up to 64-bit)
    __int128 result = static_cast<__int128>(a) * static_cast<__int128>(b);
    if (result < static_cast<__int128>(std::numeric_limits<T>::lowest())) {
        return std::numeric_limits<T>::lowest();
    } else if (result > static_cast<__int128>(std::numeric_limits<T>::max())) {
        return std::numeric_limits<T>::max();
    }
    return static_cast<T>(result);
#else
    // Fallback: widening for small types, division checks for long long
    if (sizeof(T) < sizeof(long long)) {
        auto result = static_cast<long long>(a) * static_cast<long long>(b);
        if (result < static_cast<long long>(std::numeric_limits<T>::lowest())) {
            return std::numeric_limits<T>::lowest();
        } else if (result > static_cast<long long>(std::numeric_limits<T>::max())) {
            return std::numeric_limits<T>::max();
        }
        return static_cast<T>(result);
    } else {
        // For long long itself (or __int128 if that's T), use division checks
        // Check all four sign combinations
        if (a > 0) {
            if (b > 0) {
                if (a > std::numeric_limits<T>::max() / b) {
                    return std::numeric_limits<T>::max();
                }
            } else {
                if (b < std::numeric_limits<T>::lowest() / a) {
                    return std::numeric_limits<T>::lowest();
                }
            }
        } else {
            if (b > 0) {
                if (a < std::numeric_limits<T>::lowest() / b) {
                    return std::numeric_limits<T>::lowest();
                }
            } else {
                if (a < std::numeric_limits<T>::max() / b) {
                    return std::numeric_limits<T>::max();
                }
            }
        }
        return a * b;
    }
#endif
#endif
}

/**
 * Saturating division for floating-point types
 *
 * Divides two floating-point values and clamps the result to the
 * representable range if overflow or underflow occurs.
 *
 * Division by zero uses sign-aware saturation (matches MATLAB's approach):
 * - positive / 0 → max (matches limit as divisor approaches 0+)
 * - negative / 0 → lowest (matches limit as divisor approaches 0+)
 * - 0 / 0 → 0 (neutral value for indeterminate form)
 * - NaN result → 0 (neutral value for invalid operations)
 *
 * @tparam T Floating-point type (float, double, long double)
 * @param a Dividend
 * @param b Divisor
 * @return Quotient of a and b, clamped to [lowest, max]
 */
template <typename T>
EnableFloatingPoint<T>
saturating_div(T a, T b) noexcept
{
    // Division by zero: sign-aware saturation
    // Use std::signbit() to handle negative zero correctly
    if (b == static_cast<T>(0.0)) {
        bool divisor_negative = std::signbit(b);
        if (a > static_cast<T>(0.0)) {
            return divisor_negative ?
                std::numeric_limits<T>::lowest() :
                std::numeric_limits<T>::max();
        } else if (a < static_cast<T>(0.0)) {
            return divisor_negative ?
                std::numeric_limits<T>::max() :
                std::numeric_limits<T>::lowest();
        } else {
            // 0.0 / 0.0 is indeterminate: return neutral value (0)
            return static_cast<T>(0.0);
        }
    }

    T result = a / b;
    if (std::isinf(result) || result > std::numeric_limits<T>::max()) {
        return std::numeric_limits<T>::max();
    }
    if (result < std::numeric_limits<T>::lowest()) {
        return std::numeric_limits<T>::lowest();
    }
    // NaN indicates invalid operation: return neutral value (0)
    if (std::isnan(result)) {
        return static_cast<T>(0.0);
    }
    return result;
}

/**
 * Saturating division for unsigned integer types
 *
 * Divides two unsigned integers. Division never overflows for unsigned types.
 *
 * @tparam T Unsigned integer type
 * @param a Dividend
 * @param b Divisor
 * @return Quotient of a and b
 */
template <typename T>
EnableUnsigned<T>
saturating_div(T a, T b) noexcept
{
    // Division by zero: saturate to max for consistency with overflow behavior
    if (b == 0) {
        if (a == 0) {
            return 0;  // Match signed/float behavior for 0/0
        }
        return std::numeric_limits<T>::max();
    }
    // Division never overflows for unsigned (when divisor is non-zero)
    return a / b;
}

/**
 * Saturating division for signed integer types
 *
 * Divides two signed integers and clamps the result to the maximum
 * representable value if overflow occurs (INT_MIN / -1).
 *
 * Division by zero uses sign-aware saturation (matches MATLAB's approach):
 * - positive / 0 → max (matches limit as divisor approaches 0+)
 * - negative / 0 → lowest (matches limit as divisor approaches 0+)
 * - 0 / 0 → 0 (neutral value for indeterminate form)
 *
 * @tparam T Signed integer type
 * @param a Dividend
 * @param b Divisor
 * @return Quotient of a and b, clamped to max on overflow
 */
template <typename T>
EnableSigned<T>
saturating_div(T a, T b) noexcept
{
    // Division by zero: sign-aware saturation
    // Matches limit behavior as divisor approaches zero
    if (b == 0) {
        if (a > 0) {
            return std::numeric_limits<T>::max();
        } else if (a < 0) {
            return std::numeric_limits<T>::lowest();
        } else {
            // 0 / 0 is indeterminate: return neutral value (0)
            return 0;
        }
    }
    // Only overflow case: INT_MIN / -1
    if (a == std::numeric_limits<T>::lowest() && b == static_cast<T>(-1)) {
        return std::numeric_limits<T>::max();
    }
    return a / b;
}

/**
 * Saturating remainder for unsigned integer types
 *
 * Computes the remainder of two unsigned integers. Remainder never overflows
 * for unsigned types, but we handle modulo by zero.
 *
 * Remainder by zero behavior:
 * - a % 0 → 0 (neutral value for undefined operation)
 *
 * @tparam T Unsigned integer type
 * @param a Dividend
 * @param b Divisor (modulus)
 * @return Remainder of a and b
 */
template <typename T>
EnableUnsigned<T>
saturating_rem(T a, T b) noexcept
{
    // Remainder by zero: return neutral value (0)
    if (b == 0) {
        return 0;
    }
    // Remainder never overflows for unsigned (when divisor is non-zero)
    return a % b;
}

/**
 * Saturating remainder for signed integer types
 *
 * Computes the remainder of two signed integers. Handles the edge case
 * of INT_MIN % -1, which on some architectures can trigger overflow
 * (though mathematically the result is 0).
 *
 * Remainder by zero behavior:
 * - a % 0 → 0 (neutral value for undefined operation)
 *
 * Special cases:
 * - INT_MIN % -1 → 0 (mathematical result, avoiding potential overflow)
 *
 * @tparam T Signed integer type
 * @param a Dividend
 * @param b Divisor (modulus)
 * @return Remainder of a and b
 */
template <typename T>
EnableSigned<T>
saturating_rem(T a, T b) noexcept
{
    // Remainder by zero: return neutral value (0)
    if (b == 0) {
        return 0;
    }
    // Edge case: INT_MIN % -1 can overflow on some architectures
    // Mathematically, the result is 0
    if (a == std::numeric_limits<T>::lowest() && b == static_cast<T>(-1)) {
        return 0;
    }
    return a % b;
}

// Modulo for floating-point - not provided (modulo is only defined for integral types)

} // namespace atlas_detail
} // namespace atlas

#endif // WJH_ATLAS_64A9A0E1C2564622BBEAE087A98B793D
)";

    static constexpr char const constraints_helpers[] = R"__(
#ifndef WJH_ATLAS_173D2C4FC9AA46929AD14C8BDF75D829
#define WJH_ATLAS_173D2C4FC9AA46929AD14C8BDF75D829

#include <sstream>

#ifdef __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wweak-vtables"
#endif

namespace atlas {

/**
 * @brief Exception thrown when a constraint is violated
 */
class ConstraintError
: public std::logic_error
{
public:
    using std::logic_error::logic_error;
};

namespace constraints {

namespace detail {

template <typename T>
std::string
format_value_impl(T const &, atlas_detail::PriorityTag<0>)
{
    return "unknown value";
}

template <typename T>
auto
format_value_impl(T const & value, atlas_detail::PriorityTag<1>)
-> decltype(std::declval<std::ostringstream &>() << value, std::string())
{
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

template <typename T>
auto
format_value_impl(T const & value, atlas_detail::PriorityTag<2>)
-> decltype(std::to_string(value))
{
    return std::to_string(value);
}

template <typename T>
std::string
format_value(T const & value)
{
    return format_value_impl(value, atlas_detail::PriorityTag<2>{});
}

inline int uncaught_exceptions() noexcept
{
#if defined(__cpp_lib_uncaught_exceptions) && \
    __cpp_lib_uncaught_exceptions >= 201411L
    return std::uncaught_exceptions();
#elif defined(_MSC_VER)
    return __uncaught_exceptions();  // MSVC extension available since VS2015
#elif defined(__GLIBCXX__)
    // libstdc++ has __cxa_get_globals which tracks uncaught exceptions
    return __cxxabiv1::__cxa_get_globals()->uncaughtExceptions;
#elif defined(_LIBCPP_VERSION)
    // libc++ has std::uncaught_exceptions even in C++11 mode as extension
    return std::uncaught_exceptions();
#else
    // Fallback: use old uncaught_exception() (singular) - less safe but works
    // This will return 1 during any exception, 0 otherwise
    // Can't distinguish between multiple exceptions, but better than nothing
    return std::uncaught_exception() ? 1 : 0;
#endif
}

/**
 * @brief RAII guard for validating constraints after mutating operations
 *
 * This guard validates constraints in its destructor, ensuring that the
 * constraint is checked after the operation completes. The guard checks
 * uncaught_exceptions() to avoid throwing during stack unwinding.
 *
 * Only validates non-const operations - const operations cannot violate
 * constraints by definition.
 *
 * @tparam T The value type being constrained (may be const)
 * @tparam ConstraintT The constraint type with static check() and message()
 */
template <typename T, typename ConstraintT, typename = void>
struct ConstraintGuard
{
    using value_type = typename std::remove_const<T>::type;

    T const & value;
    char const * operation_name;
    int uncaught_at_entry;

    /**
     * @brief Construct guard, capturing current exception state
     */
    constexpr ConstraintGuard(T const & v, char const * op) noexcept
    : value(v)
    , operation_name(op)
    , uncaught_at_entry(uncaught_exceptions())
    { }

    /**
     * @brief Destructor validates constraint if no new exceptions
     *
     * Only throws if the constraint is violated AND no exceptions are
     * currently unwinding (to avoid std::terminate).
     *
     * Only validates non-const operations - uses std::is_const to check.
     */
    constexpr ~ConstraintGuard() noexcept(false)
    {
        if (uncaught_exceptions() == uncaught_at_entry) {
            if (not ConstraintT::check(value)) {
                throw atlas::ConstraintError(
                    std::string(operation_name) +
                    ": operation violates constraint (" +
                    ConstraintT::message() + ")");
            }
        }
    }
};

template <typename T, typename ConstraintT>
struct ConstraintGuard<
    T,
    ConstraintT,
    typename std::enable_if<std::is_const<T>::value>::type>
{
    constexpr ConstraintGuard(T const &, char const *) noexcept
    { }
};

} // namespace detail

template <typename ConstraintT, typename T>
auto constraint_guard(T & t, char const * op) noexcept
{
    return detail::ConstraintGuard<T, ConstraintT>(t, op);
}

template <typename T>
constexpr auto is_nil_value(typename T::atlas_value_type const * value)
-> decltype(atlas::to_underlying(T::nil_value) == *value)
{
    return atlas::to_underlying(T::nil_value) == *value;
}

template <typename T>
constexpr bool is_nil_value(void const *)
{
    return false;
}

template <typename T>
constexpr bool check(typename T::atlas_value_type const & value)
{
    return is_nil_value<T>(std::addressof(value)) ||
        T::atlas_constraint::check(value);
}

/**
 * @brief Constraint: value must be > 0
 */
template <typename T>
struct positive
{
    static constexpr bool check(T const & value)
    noexcept(noexcept(value > T{0}))
    {
        return value > T{0};
    }

    static constexpr char const * message() noexcept
    {
        return "value must be positive (> 0)";
    }
};

/**
 * @brief Constraint: value must be >= 0
 */
template <typename T>
struct non_negative
{
    static constexpr bool check(T const & value)
    noexcept(noexcept(value >= T{0}))
    {
        return value >= T{0};
    }

    static constexpr char const * message() noexcept
    {
        return "value must be non-negative (>= 0)";
    }
};

/**
 * @brief Constraint: value must be != 0
 */
template <typename T>
struct non_zero
{
    static constexpr bool check(T const & value)
    noexcept(noexcept(value != T{0}))
    {
        return value != T{0};
    }

    static constexpr char const * message() noexcept
    {
        return "value must be non-zero (!= 0)";
    }
};

/**
 * Constraint: value must be in [Min, Max]
 */
template <typename T>
struct bounded
{
    static constexpr bool check(typename T::value_type const & value)
    noexcept(noexcept(value >= T::min()) && noexcept(value <= T::max()))
    {
        return value >= T::min() && value <= T::max();
    }

    static constexpr char const * message() noexcept
    {
        return T::message();
    }
};

/**
 * Constraint: value must be in [Min, Max) (half-open range)
 */
template <typename T>
struct bounded_range
{
    static constexpr bool check(typename T::value_type const & value)
    noexcept(noexcept(value >= T::min()) && noexcept(value < T::max()))
    {
        return value >= T::min() && value < T::max();
    }

    static constexpr char const * message() noexcept
    {
        return T::message();
    }
};

/**
 * @brief Constraint: container/string must not be empty
 */
template <typename T>
struct non_empty
{
    static constexpr bool check(T const & value)
    noexcept(noexcept(value.empty()))
    {
        return not value.empty();
    }

    static constexpr char const * message() noexcept
    {
        return "value must not be empty";
    }
};

/**
 * @brief Constraint: pointer must not be null
 *
 * Works with raw pointers, smart pointers (unique_ptr, shared_ptr), and
 * std::optional by using explicit bool conversion (operator bool()).
 *
 * Note: weak_ptr requires C++23 for operator bool() support.
 */
template <typename T>
struct non_null
{
    static constexpr bool check(T const & value)
    noexcept(noexcept(static_cast<bool>(value)))
    {
        // Use explicit bool conversion - works for:
        // - Raw pointers (void*, int*, etc.)
        // - Smart pointers (unique_ptr, shared_ptr)
        // - std::optional
        // - Any type with explicit operator bool()
        return static_cast<bool>(value);
    }

    static constexpr char const * message() noexcept
    {
        return "pointer must not be null";
    }
};

} // namespace constraints
} // namespace atlas

#ifdef __clang__
    #pragma clang diagnostic pop
#endif

#endif // WJH_ATLAS_173D2C4FC9AA46929AD14C8BDF75D829
)__";

    static constexpr char const optional_support[] = R"(
#ifndef WJH_ATLAS_04D0CC2BF798478DBE3CA9BFFCC24233
#define WJH_ATLAS_04D0CC2BF798478DBE3CA9BFFCC24233

namespace atlas {

template <typename T, typename = void>
struct can_be_nilable
: std::false_type
{ };

template <typename T>
struct can_be_nilable<
    T,
    typename std::enable_if<std::is_same<
        remove_cv_t<T>,
        remove_cv_t<decltype(T::nil_value)>>::value>::type>
: std::true_type
{ };

#ifdef __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wweak-vtables"
#endif

/**
 * Exception thrown when an atlas::Nilable is accessed without a value.
 */
class BadNilableAccess
: public std::logic_error
{
public:
    using std::logic_error::logic_error;
    explicit BadNilableAccess()
    : std::logic_error("bad atlas::Nilable access")
    { }
};

#ifdef __clang__
    #pragma clang diagnostic pop
#endif

namespace detail {

template <typename T, typename = void>
class BasicNilable;

template <typename T>
class BasicNilable<T, typename std::enable_if<can_be_nilable<T>::value>::type>
{
    T value_;

public:
    explicit BasicNilable() = default;

    BasicNilable(BasicNilable const &) = default;

    BasicNilable & operator = (BasicNilable const &) = default;

#if __cplusplus >= 201402L
    constexpr
#endif
    BasicNilable(BasicNilable && other) noexcept(
        std::is_nothrow_move_constructible<T>::value &&
        std::is_nothrow_copy_assignable<T>::value)
    : value_(std::move(other.value_))
    {
        other.value_ = T::nil_value;
    }

#if __cplusplus >= 201402L
    constexpr
#endif
    BasicNilable & operator = (BasicNilable && other) noexcept(
        std::is_nothrow_move_assignable<T>::value &&
        std::is_nothrow_copy_assignable<T>::value)
    {
        if (this != std::addressof(other)) {
            value_ = std::move(other.value_);
            other.value_ = T::nil_value;
        }
        return *this;
    }


    constexpr explicit BasicNilable(std::nullopt_t) noexcept(
        std::is_nothrow_copy_constructible<T>::value)
    : value_(T::nil_value)
    { }

    template <
        typename U,
        when<
            std::is_constructible<T, U>::value &&
            not std::is_convertible<U, T>::value> = true>
    constexpr explicit BasicNilable(U && u)
    : value_(std::forward<U>(u))
    { }

    template <typename U, when<std::is_convertible<U, T>::value> = true>
    constexpr BasicNilable(U && u)
    : value_(std::forward<U>(u))
    { }

    template <
        typename... ArgTs,
        when<std::is_constructible<T, ArgTs...>::value> = true>
    constexpr explicit BasicNilable(
        std::in_place_t,
        ArgTs &&... args) noexcept(std::is_nothrow_constructible<T, ArgTs...>::
                                       value)
    : value_(std::forward<ArgTs>(args)...)
    { }

    constexpr explicit operator bool () const noexcept
    {
        return not (atlas::to_underlying(value_) == atlas::to_underlying(T::nil_value));
    }

    constexpr bool has_value() const noexcept { return bool(*this); }

    constexpr T * operator -> () noexcept { return std::addressof(value_); }

    constexpr T const * operator -> () const noexcept
    {
        return std::addressof(value_);
    }

    constexpr T const & operator * () const & noexcept { return value_; }

    constexpr T & operator * () & noexcept { return value_; }

    constexpr T const && operator * () const && noexcept
    {
        return static_cast<T const &&>(value_);
    }

    constexpr T && operator * () && noexcept
    {
        return static_cast<T &&>(value_);
    }

#if __cplusplus >= 201402L
    #define WJH_ATLAS_tmp constexpr
#else
    #define WJH_ATLAS_tmp
#endif
    WJH_ATLAS_tmp T & value() &
    {
        if (has_value()) {
            return value_;
        }
        throw BadNilableAccess();
    }

    WJH_ATLAS_tmp T const & value() const &
    {
        if (has_value()) {
            return value_;
        }
        throw BadNilableAccess();
    }

    WJH_ATLAS_tmp T && value() &&
    {
        if (has_value()) {
            return static_cast<T &&>(value_);
        }
        throw BadNilableAccess();
    }

    WJH_ATLAS_tmp T const && value() const &&
    {
        if (has_value()) {
            return static_cast<T const &&>(value_);
        }
        throw BadNilableAccess();
    }

#undef WJH_ATLAS_tmp
};

} // namespace detail

template <typename T>
class Nilable
: public detail::BasicNilable<T>
{
public:
    using detail::BasicNilable<T>::BasicNilable;

    Nilable(Nilable const &) = default;
    Nilable(Nilable &&) = default;
    Nilable & operator = (Nilable const &) = default;
    Nilable & operator = (Nilable &&) = default;

    Nilable & operator = (std::nullopt_t)
    {
        **this = T::nil_value;
        return *this;
    }

    Nilable & operator = (T const & t)
    {
        **this = t;
        return *this;
    }

    Nilable & operator = (T && t) noexcept
    {
        **this = std::move(t);
        return *this;
    }

    void swap(Nilable & that) noexcept
    {
        auto & self = *this;
        if (self.has_value()) {
            if (that.has_value()) {
                using std::swap;
                swap(*self, *that);
            } else {
                *that = std::move(*self);
                self = T::nil_value;
            }
        } else if (that.has_value()) {
            *self = std::move(*that);
            that = T::nil_value;
        }
    }

    void reset() noexcept { *this = T::nil_value; }

    template <
        typename... ArgTs,
        when<std::is_constructible<T, ArgTs...>::value> = true>
    T & emplace(ArgTs &&... args) noexcept(
        std::is_nothrow_constructible<T, ArgTs...>::value)
    {
        *this = T(std::forward<ArgTs>(args)...);
        return **this;
    }

    template <typename U = remove_cv_t<T>>
    constexpr enable_if_t<
        std::is_copy_constructible<T>::value &&
            std::is_convertible<U &&, T>::value,
        T>
    value_or(U && default_value) const & noexcept(
        std::is_nothrow_copy_constructible<T>::value)
    {
        if (this->has_value()) {
            return **this;
        } else {
            return static_cast<T>(std::forward<U>(default_value));
        }
    }

    template <typename U = remove_cv_t<T>>
    constexpr enable_if_t<
        std::is_move_constructible<T>::value &&
            std::is_convertible<U &&, T>::value,
        T>
    value_or(U && default_value) && noexcept(
        std::is_nothrow_move_constructible<T>::value)
    {
        if (this->has_value()) {
            return T(std::move(**this));
        } else {
            return static_cast<T>(std::forward<U>(default_value));
        }
    }

private:
#if defined(__cpp_lib_invoke) && (__cpp_lib_invoke >= 201411L) && \
    defined(__cpp_lib_is_invocable) && (__cpp_lib_is_invocable >= 201703L)
    template <
        typename SelfT,
        typename F,
        typename R = remove_cvref_t<std::invoke_result_t<
            F,
            decltype(std::declval<SelfT>().operator * ())>>>
    static constexpr R and_then_(SelfT && self, F && f)
    {
        if (self.has_value()) {
            return std::invoke(
                std::forward<F>(f),
                std::forward<SelfT>(self).operator * ());
        } else {
            return R{};
        }
    }
#else
    template <
        typename SelfT,
        typename F,
        typename R = remove_cvref_t<
            decltype(std::forward<F>(f)(std::declval<SelfT>().operator * ()))>>
    static constexpr R and_then_(SelfT && self, F && f)
    {
        if (self.has_value()) {
            return std::forward<F>(f)(std::forward<SelfT>(self).operator * ());
        } else {
            return R{};
        }
    }
#endif

public:
    template <typename F>
    constexpr auto and_then(F && f) &
    {
        return and_then_(*this, std::forward<F>(f));
    }

    template <typename F>
    constexpr auto and_then(F && f) const &
    {
        return and_then_(*this, std::forward<F>(f));
    }

    template <typename F>
    constexpr auto and_then(F && f) &&
    {
        return and_then_(std::move(*this), std::forward<F>(f));
    }

    template <typename F>
    constexpr auto and_then(F && f) const &&
    {
        return and_then_(std::move(*this), std::forward<F>(f));
    }

    template <typename F>
    constexpr auto or_else(F && f) const &
    -> decltype(this->has_value() ? *this : std::forward<F>(f)())
    {
        return this->has_value() ? *this : std::forward<F>(f)();
    }

    template <typename F>
    constexpr auto or_else(F && f) &&
    -> decltype(this->has_value() ? std::move(*this) : std::forward<F>(f)())
    {
        return this->has_value() ? std::move(*this) : std::forward<F>(f)();
    }

private:
    template <typename U>
    static constexpr std::true_type matches_opt_(Nilable<U> const &);
    template <typename U>
    static constexpr std::true_type matches_opt_(std::optional<U> const &);
    static constexpr std::false_type matches_opt_(...);

    template <typename U>
    struct is_an_optional
    : decltype(matches_opt_(std::declval<U const &>()))
    { };

    // Helper to check if a type is our strong type (exactly)
    // We want to exclude random types (like doctest expression templates)
    // and also not allow Nilable<T> to match (which would be convertible to T)
    template <typename U, typename = void>
    struct is_value_comparable
    : std::false_type
    { };

    template <typename U>
    struct is_value_comparable<
        U,
        typename std::enable_if<
            not is_an_optional<U>::value &&
            not std::is_same<U, std::nullopt_t>::value &&
            std::is_same<remove_cvref_t<U>, T>::value>::type>
    : std::true_type
    { };

    // Helper to check if T and U are equality comparable
    template <typename TT, typename UU, typename = void>
    struct is_equality_comparable
    : std::false_type
    { };

    template <typename TT, typename UU>
    struct is_equality_comparable<
        TT,
        UU,
        typename std::enable_if<std::is_convertible<
            decltype(std::declval<TT const &>() == std::declval<UU const &>()),
            bool>::value>::type>
    : std::true_type
    { };

    // Helper to check if T and U are less-than comparable
    template <typename TT, typename UU, typename = void>
    struct is_less_comparable
    : std::false_type
    { };

    template <typename TT, typename UU>
    struct is_less_comparable<
        TT,
        UU,
        typename std::enable_if<std::is_convertible<
            decltype(std::declval<TT const &>() < std::declval<UU const &>()),
            bool>::value>::type>
    : std::true_type
    { };

    template <typename X, typename Y>
    static constexpr bool equal_(X const & x, Y const & y)
    {
        if (x.has_value()) {
            if (y.has_value()) {
                return bool(*x == *y);
            } else {
                return false;
            }
        } else {
            return not y.has_value();
        }
    }

    template <typename U, when<is_equality_comparable<T, U>::value> = true>
    friend constexpr auto operator == (
        Nilable const & x,
        Nilable<U> const & y)
    -> decltype(bool(*x == *y))
    {
        return equal_(x, y);
    }

    template <typename U, when<is_equality_comparable<T, U>::value> = true>
    friend constexpr auto operator == (
        Nilable const & x,
        std::optional<U> const & y)
    -> decltype(bool(*x == *y))
    {
        return equal_(x, y);
    }

    friend constexpr bool operator == (Nilable const & x, std::nullopt_t)
    {
        return not x.has_value();
    }

    template <typename Y, when<is_value_comparable<Y>::value> = true>
    friend constexpr auto operator == (Nilable const & x, Y const & y)
    -> decltype(bool(*x == y))
    {
        if (x.has_value()) {
            return bool(*x == y);
        } else {
            return false;
        }
    }

#if defined(__cpp_impl_three_way_comparison) && \
    (__cpp_impl_three_way_comparison >= 201907) && \
    defined(__cpp_lib_three_way_comparison) && \
    (__cpp_lib_three_way_comparison >= 201907)

    template <typename X, typename Y>
    static constexpr auto spaceship_(X const & x, Y const & y)
    {
        if (x.has_value() && y.has_value()) {
            return *x <=> *y;
        } else {
            return x.has_value() <=> y.has_value();
        }
    }

    template <std::three_way_comparable_with<T> U>
    friend constexpr std::compare_three_way_result_t<T, U> operator <=> (
        Nilable const & x,
        Nilable<U> const & y)
    {
        return spaceship_(x, y);
    }

    template <std::three_way_comparable_with<T> U>
    friend constexpr std::compare_three_way_result_t<T, U> operator <=> (
        Nilable const & x,
        std::optional<U> const & y)
    {
        return spaceship_(x, y);
    }

    friend constexpr std::strong_ordering operator <=> (
        Nilable const & x,
        std::nullopt_t) noexcept
    {
        return x.has_value() <=> false;
    }

    template <typename Y>
    requires(not is_an_optional<Y>::value) &&
        std::three_way_comparable_with<T, Y>
    friend constexpr std::compare_three_way_result_t<T, Y> operator <=> (
        Nilable const & x,
        Y const & y)
    {
        return x.has_value() ? *x <=> y : std::strong_ordering::less;
    }

#endif

    // C++11/17 comparison operators - also used as fallback in C++20
    // when T doesn't support spaceship
    template <
        typename U,
        when<
            not std::is_same<U, T>::value &&
            is_equality_comparable<T, U>::value> = true>
    friend constexpr auto operator == (
        Nilable<U> const & x,
        Nilable const & y)
    -> decltype(bool(*x == *y))
    {
        return equal_(x, y);
    }

    template <typename U, when<is_equality_comparable<T, U>::value> = true>
    friend constexpr auto operator == (
        std::optional<U> const & x,
        Nilable const & y)
    -> decltype(bool(*x == *y))
    {
        return equal_(x, y);
    }

    friend constexpr bool operator == (std::nullopt_t, Nilable const & y)
    {
        return not y.has_value();
    }

    template <typename X, when<is_value_comparable<X>::value> = true>
    friend constexpr auto operator == (X const & x, Nilable const & y)
    -> decltype(bool(x == *y))
    {
        if (y.has_value()) {
            return bool(x == *y);
        } else {
            return false;
        }
    }

    template <typename X, typename Y>
    static constexpr bool less_(X const & x, Y const & y)
    {
        if (x.has_value()) {
            if (y.has_value()) {
                return bool(*x < *y);
            } else {
                return false;
            }
        } else {
            return y.has_value();
        }
    }

    template <typename U, when<is_less_comparable<T, U>::value> = true>
    friend constexpr auto operator < (Nilable const & x, Nilable<U> const & y)
    -> decltype(bool(*x < *y))
    {
        return less_(x, y);
    }

    template <typename U, when<is_less_comparable<T, U>::value> = true>
    friend constexpr auto operator < (
        Nilable const & x,
        std::optional<U> const & y)
    -> decltype(bool(*x < *y))
    {
        return less_(x, y);
    }

    template <
        typename U,
        when<not std::is_same<U, T>::value && is_less_comparable<T, U>::value> =
            true>
    friend constexpr auto operator < (Nilable<U> const & x, Nilable const & y)
    -> decltype(bool(*x < *y))
    {
        return less_(x, y);
    }

    template <typename U, when<is_less_comparable<T, U>::value> = true>
    friend constexpr auto operator < (
        std::optional<U> const & x,
        Nilable const & y)
    -> decltype(bool(*x < *y))
    {
        return less_(x, y);
    }

    friend constexpr bool operator < (Nilable const & x, std::nullopt_t)
    {
        return false;
    }

    friend constexpr bool operator < (std::nullopt_t, Nilable const & y)
    {
        return y.has_value();
    }

    template <
        typename Y,
        when<is_value_comparable<Y>::value && is_less_comparable<T, Y>::value> =
            true>
    friend constexpr auto operator < (Nilable const & x, Y const & y)
    -> decltype(bool(*x < y))
    {
        if (x.has_value()) {
            return bool(*x < y);
        } else {
            return true;
        }
    }

    template <
        typename X,
        when<is_value_comparable<X>::value && is_less_comparable<X, T>::value> =
            true>
    friend constexpr auto operator < (X const & x, Nilable const & y)
    -> decltype(bool(x < *y))
    {
        if (y.has_value()) {
            return bool(x < *y);
        } else {
            return false;
        }
    }

    template <typename U>
    using is_me = std::is_same<U, Nilable>;

// In C++20, != is synthesized from ==
// Only provide it explicitly in pre-C++20
#if not defined(__cpp_impl_three_way_comparison) || \
    (__cpp_impl_three_way_comparison < 201907)

    template <typename Y>
    friend constexpr auto operator != (Nilable const & x, Y const & y)
    -> decltype(not (x == y))
    {
        return not (x == y);
    }

    template <typename X, when<not is_me<X>::value> = true>
    friend constexpr auto operator != (X const & x, Nilable const & y)
    -> decltype(not (x == y))
    {
        return not (x == y);
    }

#endif

    // In C++20, >, <=, >= CAN be synthesized from <=> if it exists
    // BUT if <=> doesn't exist, they need to be defined explicitly
    // So we provide them in ALL modes, defined in terms of <
    template <typename Y>
    friend constexpr auto operator > (Nilable const & x, Y const & y)
    -> decltype(y < x)
    {
        return y < x;
    }

    template <typename X, when<not is_me<X>::value> = true>
    friend constexpr auto operator > (X const & x, Nilable const & y)
    -> decltype(y < x)
    {
        return y < x;
    }

    template <typename Y>
    friend constexpr auto operator <= (Nilable const & x, Y const & y)
    -> decltype(not (y < x))
    {
        return not (y < x);
    }

    template <typename X, when<not is_me<X>::value> = true>
    friend constexpr auto operator <= (X const & x, Nilable const & y)
    -> decltype(not (y < x))
    {
        return not (y < x);
    }

    template <typename Y>
    friend constexpr auto operator >= (Nilable const & x, Y const & y)
    -> decltype(not (x < y))
    {
        return not (x < y);
    }

    template <typename X, when<not is_me<X>::value> = true>
    friend constexpr auto operator >= (X const & x, Nilable const & y)
    -> decltype(not (x < y))
    {
        return not (x < y);
    }
};
} // namespace atlas

template <typename T>
struct std::hash<atlas::Nilable<T>>
{
private:
    // Hash the underlying value type, not the strong type wrapper
    using value_type = typename T::atlas_value_type;

public:
    auto operator () (atlas::Nilable<T> const & x) const noexcept(
        noexcept(std::hash<value_type>{}(std::declval<value_type const &>())))
    -> decltype(std::hash<value_type>{}(std::declval<value_type const &>()))
    {
        if (x.has_value()) {
            return std::hash<value_type>{}(atlas::to_underlying(*x));
        } else {
            return std::hash<value_type *>{}(nullptr);
        }
    }
};

#endif // WJH_ATLAS_04D0CC2BF798478DBE3CA9BFFCC24233
)";

    static constexpr char const droids[] = R"(

//////////////////////////////////////////////////////////////////////
///
/// These are the droids you are looking for!
///
//////////////////////////////////////////////////////////////////////

)";

    std::string result = basic + 1;
    if (options.include_arrow_operator_traits ||
        options.include_dereference_operator_traits)
    {
        result += const_mutable;
    }
    if (options.include_arrow_operator_traits) {
        result += arrow_helpers;
    }
    if (options.include_dereference_operator_traits) {
        result += star_helpers;
    }
    if (options.include_checked_helpers) {
        result += checked_helpers;
    }
    if (options.include_saturating_helpers) {
        result += saturating_helpers;
    }
    if (options.include_constraints) {
        result += constraints_helpers;
    }
    if (options.include_nilable_support) {
        result += optional_support;
    }
    result += droids;
    return result;
}

bool
supports_color(int fd)
{
    // Check if output is a TTY
    if (not isatty(fd)) {
        return false;
    }

    // Check TERM environment variable
    auto term = std::getenv("TERM");
    if (not term) {
        return false;
    }

    std::string term_str(term);
    // Most modern terminals support color
    return term_str != "dumb";
}

int
parse_cpp_standard(std::string_view val)
{
    // Remove "c++" or "C++" prefix if present
    if (val.size() >= 3 &&
        (val.substr(0, 3) == "c++" || val.substr(0, 3) == "C++"))
    {
        val.remove_prefix(3);
    }

    // Parse the numeric value
    int standard = 0;
    try {
        standard = std::stoi(std::string(val));
    } catch (...) {
        throw std::invalid_argument(
            "Invalid C++ standard format: '" + std::string(val) +
            "'\nValid formats: 11, 14, 17, 20, 23, c++20, C++20");
    }

    // Validate it's a supported standard
    if (standard != 11 && standard != 14 && standard != 17 && standard != 20 &&
        standard != 23)
    {
        throw std::invalid_argument(
            "Unsupported C++ standard: " + std::to_string(standard) +
            "\nValid values: 11, 14, 17, 20, 23");
    }

    return standard;
}

std::string
generate_cpp_standard_assertion(int standard)
{
    // C++11 is the minimum, no assertion needed
    if (standard == 11) {
        return "";
    }

    // Map standard to __cplusplus value
    long cpp_value;
    std::string standard_name;

    switch (standard) {
    case 14:
        cpp_value = 201402L;
        standard_name = "C++14";
        break;
    case 17:
        cpp_value = 201703L;
        standard_name = "C++17";
        break;
    case 20:
        cpp_value = 202002L;
        standard_name = "C++20";
        break;
    case 23:
        cpp_value = 202302L;
        standard_name = "C++23";
        break;
    default:
        return ""; // Should not happen if validation is done properly
    }

    std::stringstream strm;
    strm << "static_assert(__cplusplus >= " << cpp_value
        << "L,\n    \"This file requires " << standard_name
        << " or later. Compile with -std=c++" << standard
        << " or higher.\");\n\n";
    return strm.str();
}

} // namespace wjh::atlas
