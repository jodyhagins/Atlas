// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_49A3B7E8_4D91_4F26_9A10_3C8E9F6A2D47
#define WJH_ATLAS_49A3B7E8_4D91_4F26_9A10_3C8E9F6A2D47

#include <boost/json/object.hpp>

#include "atlas/StrongTypeGenerator.hpp"
#include "atlas/generation/core/ClassInfo.hpp"

#include <string_view>

namespace wjh::atlas::generation { inline namespace v1 {

namespace arithmetic_helpers {

/**
 * Check if a specific binary operator is present in the description
 *
 * Parses the StrongTypeDescription and searches for the specified operator
 * symbol in the arithmetic_binary_operators list. This function performs a
 * single parse to avoid redundant parsing in operator checks.
 *
 * @param info Strong type description to parse
 * @param op_symbol The operator symbol to search for ("+", "-", "*", "/", "%")
 * @return true if the operator is enabled, false otherwise
 */
[[nodiscard]]
bool has_binary_operator(ClassInfo const & info, std::string_view op_symbol);

/**
 * Check if a binary operator is present AND matches the specified mode
 *
 * Parses the StrongTypeDescription and checks both:
 * 1. The operator symbol is in arithmetic_binary_operators
 * 2. The arithmetic_mode matches the specified mode
 *
 * This function performs a single parse with early exit optimization:
 * mode is checked first (fast), then operator presence.
 *
 * @param info Strong type description to parse
 * @param op_symbol The operator symbol to search for
 * @param mode The arithmetic mode to check
 * (Default/Checked/Saturating/Wrapping)
 * @return true if operator is enabled and mode matches
 */
[[nodiscard]]
bool has_binary_operator_with_mode(
    ClassInfo const & info,
    std::string_view op_symbol,
    ArithmeticMode mode);

/**
 * Prepare common variables for binary operator template rendering
 *
 * Creates a JSON object with standard variables needed by all binary operators.
 * This includes class name, operator symbol, underlying type, constraint info,
 * const_expr setting, and full qualified name.
 *
 * The JSON object is compatible with Mustache template rendering and contains:
 * - "class_name": Simple class name
 * - "underlying_type": The underlying primitive type
 * - "full_qualified_name": Fully qualified class name with namespaces
 * - "has_constraint": Boolean indicating if constraints are enabled
 * - "constraint_message": The constraint violation message
 * - "op": The operator symbol
 * - "const_expr": Boolean indicating if constexpr is enabled
 *
 * @param info Strong type description to parse
 * @param op_symbol The operator symbol ("+", "-", "*", "/", "%")
 * @return JSON object with Mustache template variables
 */
[[nodiscard]]
boost::json::object prepare_binary_operator_variables(
    ClassInfo const & info,
    std::string_view op_symbol);

}}} // namespace wjh::atlas::generation::v1::arithmetic_helpers

#endif // WJH_ATLAS_49A3B7E8_4D91_4F26_9A10_3C8E9F6A2D47
