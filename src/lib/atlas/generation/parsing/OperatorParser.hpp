// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_F7A6B9C3D8E4F1A2B7C6D5E4F3A2B1C0
#define WJH_ATLAS_F7A6B9C3D8E4F1A2B7C6D5E4F3A2B1C0

#include <array>
#include <string>
#include <string_view>

namespace wjh::atlas::generation { inline namespace v1 {

/**
 * @brief Arithmetic computation modes for operator generation
 *
 * Defines how arithmetic operations handle edge cases like overflow:
 * - Default: Standard C++ semantics (unchecked, may overflow)
 * - Checked: Throws exception on overflow/underflow
 * - Saturating: Clamps results to type bounds
 * - Wrapping: Explicit modular arithmetic (wraps around)
 */
enum class ArithmeticMode
{
    Default, // Normal unchecked arithmetic
    Checked, // Throw on overflow
    Saturating, // Clamp to bounds
    Wrapping // Explicit wraparound
};

}} // namespace wjh::atlas::generation::v1

// BOOST_DESCRIBE_ENUM must be at namespace scope
#include <boost/describe/enum.hpp>
BOOST_DESCRIBE_ENUM(
    wjh::atlas::generation::v1::ArithmeticMode,
    Default,
    Checked,
    Saturating,
    Wrapping)

namespace wjh::atlas::generation { inline namespace v1 {

/**
 * @brief Parses and classifies operator specifications for strong type
 * generation
 *
 * The OperatorParser provides utilities for recognizing and categorizing
 * operator tokens from strong type descriptions. It handles:
 * - Arithmetic operators (binary and unary)
 * - Relational/comparison operators
 * - Cast operator syntax parsing
 *
 * Design philosophy:
 * - All functions are static: no state management required
 * - constexpr where possible: enable compile-time evaluation
 * - String-view based: avoid unnecessary allocations
 * - Clear categorization: each operator type has its own classifier
 *
 * Example usage:
 * @code
 * // Check if token is an arithmetic operator
 * if (OperatorParser::is_arithmetic_binary_operator("+")) {
 *     // Generate addition operator code
 * }
 *
 * // Parse cast operator syntax
 * bool is_implicit = false;
 * std::string type = OperatorParser::parse_cast_syntax("cast<int>",
 * is_implicit);
 * // type == "int", is_implicit == false
 * @endcode
 */
class OperatorParser
{
public:
    /**
     * @brief Recognized arithmetic binary operators
     *
     * These operators generate both:
     * 1. Binary operator (Type op Type -> Type)
     * 2. Compound assignment (Type &= Type -> Type&)
     *
     * Special operators:
     * - "+*" generates both binary + and unary +
     * - "-*" generates both binary - and unary -
     */
    static constexpr std::array<std::string_view, 12> arithmetic_binary_op_tags{
        "+",
        "-",
        "*",
        "/",
        "%",
        "&",
        "|",
        "^",
        "<<",
        ">>",
        "+*",
        "-*"};

    /**
     * @brief Recognized arithmetic unary operators
     *
     * These operators generate unary operations (op Type -> Type):
     * - "u+": unary plus
     * - "u-": unary minus (negation)
     * - "u~": unary bitwise NOT (explicitly prefixed)
     * - "~": unary bitwise NOT (standard form)
     */
    static constexpr std::array<std::string_view, 4> arithmetic_unary_operators{
        "u+",
        "u-",
        "u~",
        "~"};

    /**
     * @brief Recognized relational/comparison operators
     *
     * These operators generate comparison functions (Type op Type -> bool):
     * - "==", "!=": equality/inequality
     * - "<", "<=", ">", ">=": ordering comparisons
     *
     * Note: The spaceship operator "<=>" is handled separately
     */
    static constexpr std::array<std::string_view, 6>
        relational_operators{"==", "!=", "<=", ">=", "<", ">"};

    /**
     * @brief Check if operator is an arithmetic binary operator
     *
     * Tests whether the given string_view matches any arithmetic binary
     * operator tag, including special forms like "+*" and "-*".
     *
     * @param sv The operator string to test
     * @return true if sv is a recognized arithmetic binary operator
     *
     * @throws None - This is a constexpr lookup function
     *
     * Examples:
     * @code
     * OperatorParser::is_arithmetic_binary_operator("+")   // true
     * OperatorParser::is_arithmetic_binary_operator("<<")  // true
     * OperatorParser::is_arithmetic_binary_operator("+*")  // true (special)
     * OperatorParser::is_arithmetic_binary_operator("++")  // false
     * OperatorParser::is_arithmetic_binary_operator("==")  // false
     * (relational)
     * @endcode
     */
    [[nodiscard]]
    static constexpr bool is_arithmetic_binary_operator(
        std::string_view sv) noexcept;

    /**
     * @brief Check if operator is an arithmetic unary operator
     *
     * Tests whether the given string_view matches any arithmetic unary
     * operator, including both prefixed ("u+") and standard ("~") forms.
     *
     * @param sv The operator string to test
     * @return true if sv is a recognized arithmetic unary operator
     *
     * @throws None - This is a constexpr lookup function
     *
     * Examples:
     * @code
     * OperatorParser::is_arithmetic_unary_operator("u+")  // true
     * OperatorParser::is_arithmetic_unary_operator("u-")  // true
     * OperatorParser::is_arithmetic_unary_operator("~")   // true
     * OperatorParser::is_arithmetic_unary_operator("+")   // false (binary)
     * OperatorParser::is_arithmetic_unary_operator("!")   // false (logical)
     * @endcode
     */
    [[nodiscard]]
    static constexpr bool is_arithmetic_unary_operator(
        std::string_view sv) noexcept;

    /**
     * @brief Check if operator is a relational/comparison operator
     *
     * Tests whether the given string_view matches any relational operator.
     * This includes both equality (==, !=) and ordering (<, <=, >, >=).
     *
     * Note: The spaceship operator "<=>" is NOT included here - it's
     * handled separately due to its special defaulted generation rules.
     *
     * @param sv The operator string to test
     * @return true if sv is a recognized relational operator
     *
     * @throws None - This is a constexpr lookup function
     *
     * Examples:
     * @code
     * OperatorParser::is_relational_operator("==")  // true
     * OperatorParser::is_relational_operator("<")   // true
     * OperatorParser::is_relational_operator("<=")  // true
     * OperatorParser::is_relational_operator("<=>") // false (spaceship is
     * special) OperatorParser::is_relational_operator("+")   // false
     * (arithmetic)
     * @endcode
     */
    [[nodiscard]]
    static constexpr bool is_relational_operator(std::string_view sv) noexcept;

    /**
     * @brief Parse cast operator syntax from description token
     *
     * Parses cast operator specifications in the form:
     * - "cast<Type>" -> explicit cast to Type
     * - "explicit_cast<Type>" -> explicit cast to Type
     * - "implicit_cast<Type>" -> implicit cast to Type (use sparingly!)
     *
     * The function extracts the target type and determines whether the
     * cast should be explicit or implicit. The is_implicit parameter
     * is set as an output parameter.
     *
     * @param token The operator token to parse (e.g., "cast<int>")
     * @param is_implicit [out] Set to true for implicit_cast, false otherwise
     * @return The target type name (e.g., "int"), or empty string if not a cast
     *
     * @throws std::invalid_argument if the syntax is invalid (missing >, etc.)
     *
     * Examples:
     * @code
     * bool implicit;
     * auto type = OperatorParser::parse_cast_syntax("cast<double>", implicit);
     * // type == "double", implicit == false
     *
     * type = OperatorParser::parse_cast_syntax("implicit_cast<bool>",
     * implicit);
     * // type == "bool", implicit == true
     *
     * type = OperatorParser::parse_cast_syntax("+", implicit);
     * // type == "" (not a cast)
     *
     * // Throws exception:
     * type = OperatorParser::parse_cast_syntax("cast<int", implicit);
     * // Missing closing '>'
     * @endcode
     *
     * @note Leading/trailing whitespace in the type name is automatically
     * stripped
     */
    [[nodiscard]]
    static std::string parse_cast_syntax(
        std::string_view token,
        bool & is_implicit);
};

// Inline constexpr implementations
constexpr bool
OperatorParser::
is_arithmetic_binary_operator(std::string_view sv) noexcept
{
    for (auto const & op : arithmetic_binary_op_tags) {
        if (sv == op) {
            return true;
        }
    }
    return false;
}

constexpr bool
OperatorParser::
is_arithmetic_unary_operator(std::string_view sv) noexcept
{
    for (auto const & op : arithmetic_unary_operators) {
        if (sv == op) {
            return true;
        }
    }
    return false;
}

constexpr bool
OperatorParser::
is_relational_operator(std::string_view sv) noexcept
{
    for (auto const & op : relational_operators) {
        if (sv == op) {
            return true;
        }
    }
    return false;
}

}} // namespace wjh::atlas::generation::v1

#endif // WJH_ATLAS_F7A6B9C3D8E4F1A2B7C6D5E4F3A2B1C0
