// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_D7F2A9B4C8E1D6F3A7B2D5E8F1A9C4D7
#define WJH_ATLAS_D7F2A9B4C8E1D6F3A7B2D5E8F1A9C4D7

#include "atlas/generation/core/ITemplate.hpp"

namespace wjh::atlas::generation { inline namespace v1 {

/**
 * Logical AND operator template (&&)
 *
 * Generates binary logical AND operator (&&) for a strong type:
 * - Returns bool result of applying && to both wrapped values
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
 * Note: Uses "and" keyword form in generated code for consistency with
 * modern C++ style guidelines.
 */
class LogicalAndOperator final
: public ITemplate
{
protected:
    [[nodiscard]]
    std::string id_impl() const override
    {
        return "operators.logical.and";
    }

    /**
     * Sort key for logical AND operator
     *
     * @return Sort key: "&&" (the logical AND operator symbol)
     */
    [[nodiscard]]
    std::string sort_key_impl() const override
    {
        return "&&";
    }

    [[nodiscard]]
    std::string_view get_template_impl() const noexcept override;

    /**
     * Check if logical AND operator is enabled
     *
     * Examines the ClassInfo to determine if the "&&" or "and" operator
     * has been requested for this strong type.
     *
     * @param info Strong type class information
     * @return true if "&&" operator is enabled
     */
    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;

    /**
     * Prepare variables for logical AND operator rendering
     *
     * Creates a JSON object with variables needed for rendering:
     * - const_expr: "constexpr" or empty string based on settings
     * - class_name: name of the strong type class
     * - underlying_type: the wrapped type
     * - op: the operator symbol ("and")
     *
     * @param info Strong type class information
     * @return JSON object with template variables
     */
    [[nodiscard]]
    boost::json::object prepare_variables_impl(
        ClassInfo const & info) const override;
};

}} // namespace wjh::atlas::generation::v1

#endif // WJH_ATLAS_D7F2A9B4C8E1D6F3A7B2D5E8F1A9C4D7
