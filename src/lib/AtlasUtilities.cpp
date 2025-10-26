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

namespace wjh::atlas { inline namespace v1 {

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
        auto n = std::min(sv.find(sep), sv.size());
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

std::string
get_sha1(std::string const & s)
{
    boost::uuids::detail::sha1 sha1;
    sha1.process_bytes(s.data(), s.size());
    boost::uuids::detail::sha1::digest_type hash;
    sha1.get_digest(hash);

    std::string result;
    for (unsigned int x : hash) {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "%08x", x);
        result += buffer;
    }
    return result;
}

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
get_preamble_includes(PreambleOptions /* options */)
{
    std::vector<std::string> includes;

    // Base includes always needed by preamble
    includes.push_back("<type_traits>");
    includes.push_back("<utility>");

    // Conditionally add <compare> if three-way comparison is supported
    // This is handled separately in the generated code via #if directive,
    // so we don't include it in the unconditional list

    // Note: options parameter reserved for future use (e.g., if we need to
    // conditionally include headers based on arrow/dereference operators)

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
// - atlas::value(): Universal value accessor for strong types
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

template <typename T, typename = void>
struct IsAtlasType
: std::false_type
{ };

template <typename T>
struct IsAtlasType<T, void_t<typename T::atlas_value_type>>
: std::true_type
{ };

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
template <typename T, typename U = void>
using enable_if = typename std::enable_if<T::value, U>::type;

template <typename T>
using _t = typename T::type;

void atlas_value();

template <typename T>
constexpr T &
value(T & val, PriorityTag<0>)
{
    return val;
}

template <typename T, typename U = typename T::atlas_value_type>
using val_t = _t<std::conditional<std::is_const<T>::value, U const &, U &>>;

template <typename T, typename U = val_t<T>>
constexpr auto
value(T & val, PriorityTag<1>)
-> decltype(atlas::atlas_detail::value(static_cast<U>(val), value_tag{}))
{
    return atlas::atlas_detail::value(static_cast<U>(val), value_tag{});
}

template <typename T>
constexpr auto
value(T const & t, PriorityTag<2>)
-> decltype(atlas_value(t, atlas::value_tag{}))
{
    return atlas_value(t, atlas::value_tag{});
}

template <typename T>
constexpr auto
value(T const & t, PriorityTag<3>)
-> decltype(atlas_value(t))
{
    return atlas_value(t);
}

class Value
{
    template <
        typename U,
        typename T,
        typename V = _t<std::conditional<is_lref<U &&>::value, T &, T>>>
    static constexpr V rval(T && t)
    {
        return t;
    }

public:
    template <typename T>
    constexpr auto operator () (T && t) const
    -> decltype(rval<T>(atlas_detail::value(t, atlas_detail::value_tag{})))
    {
        return rval<T>(atlas_detail::value(t, atlas_detail::value_tag{}));
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

#if defined(__cpp_inline_variables) && __cpp_inline_variables >= 201606L
inline constexpr auto value = atlas_detail::Value{};
#else
template <typename T>
constexpr auto
value(T && t)
-> decltype(atlas_detail::Value{}(std::forward<T>(t)))
{
    return atlas_detail::Value{}(std::forward<T>(t));
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
    virtual ~ArithmeticError() noexcept = default;
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
    virtual ~CheckedOverflowError() noexcept = default;
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
    virtual ~CheckedUnderflowError() noexcept = default;
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
    virtual ~CheckedDivisionByZeroError() noexcept = default;
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
    virtual ~CheckedInvalidOperationError() noexcept = default;
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

}} // namespace wjh::atlas::v1
