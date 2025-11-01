// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_E8F3B1C9D2E7F4A8B3D6E9F2A1C5D8E3
#define WJH_ATLAS_E8F3B1C9D2E7F4A8B3D6E9F2A1C5D8E3

#include "atlas/generation/core/ITemplate.hpp"

namespace wjh::atlas::generation { inline namespace v1 {

/**
 * Logical OR operator template (||)
 *
 * Generates binary logical OR operator (||) for a strong type:
 * - Returns bool result of applying || to both wrapped values
 * - Marked noexcept if the underlying operation is noexcept
 * - Uses friend function for proper ADL
 *
 * IMPORTANT WARNING: The generated operator includes a note that overloading
 * logical operators is generally discouraged because:
 * 1. Short-circuit evaluation is lost (both operands always evaluated)
 * 2. Can be confusing and unexpected for users
 * 3. Not idiomatic C++ for most use cases
 *
 * These operators should only be used when there's a clear, justified need.
 *
 * Note: Uses "or" keyword form in generated code for consistency with
 * modern C++ style guidelines.
 */
class LogicalOrOperator final
: public ITemplate
{
protected:
    [[nodiscard]]
    std::string id_impl() const override
    {
        return "operators.logical.or";
    }

    /**
     * Sort key for logical OR operator
     *
     * @return Sort key: "||" (the logical OR operator symbol)
     */
    [[nodiscard]]
    std::string sort_key_impl() const override
    {
        return "||";
    }

    [[nodiscard]]
    std::string_view get_template_impl() const noexcept override;

    /**
     * Check if logical OR operator is enabled
     *
     * Examines the ClassInfo to determine if the "||" or "or" operator
     * has been requested for this strong type.
     *
     * @param info Strong type class information
     * @return true if "||" operator is enabled
     */
    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;

    /**
     * Prepare variables for logical OR operator rendering
     *
     * Creates a JSON object with variables needed for rendering:
     * - const_expr: "constexpr" or empty string based on settings
     * - class_name: name of the strong type class
     * - underlying_type: the wrapped type
     * - op: the operator symbol ("or")
     *
     * @param info Strong type class information
     * @return JSON object with template variables
     */
    [[nodiscard]]
    boost::json::object prepare_variables_impl(
        ClassInfo const & info) const override;
};

}} // namespace wjh::atlas::generation::v1

#endif // WJH_ATLAS_E8F3B1C9D2E7F4A8B3D6E9F2A1C5D8E3
