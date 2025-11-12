// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_F9A3D7E2B6C4F1A8D5E3B9C2F7A4D1E6
#define WJH_ATLAS_F9A3D7E2B6C4F1A8D5E3B9C2F7A4D1E6

#include <boost/json/object.hpp>

#include <string_view>

// Forward declaration
namespace wjh::atlas::generation {
struct ClassInfo;
}

namespace wjh::atlas::generation { namespace logical_helpers {

/**
 * Check if a specific binary logical operator is enabled
 *
 * Searches the logical_operators list for the specified operator symbol.
 * Single parse to avoid redundant work.
 *
 * @param info Strong type class information
 * @param op_symbol The operator symbol to search for ("and", "or")
 * @return true if the operator is enabled
 */
[[nodiscard]]
bool has_binary_logical_operator(
    ClassInfo const & info,
    std::string_view op_symbol);

/**
 * Prepare common variables for binary logical operator template rendering
 *
 * Creates a JSON object with standard variables needed by all binary logical
 * operators. Compatible with Mustache template rendering.
 *
 * @param info Strong type class information
 * @param op_symbol The operator symbol ("and", "or")
 * @return JSON object with template variables
 */
[[nodiscard]]
boost::json::object prepare_binary_logical_operator_variables(
    ClassInfo const & info,
    std::string_view op_symbol);

/**
 * Get the shared template for binary logical operators
 *
 * Both AND and OR operators use identical template structure,
 * differentiated only by the {{{op}}} placeholder.
 *
 * @return The shared Mustache template
 */
[[nodiscard]]
std::string_view get_binary_logical_operator_template() noexcept;

}} // namespace wjh::atlas::generation::logical_helpers

#endif // WJH_ATLAS_F9A3D7E2B6C4F1A8D5E3B9C2F7A4D1E6
