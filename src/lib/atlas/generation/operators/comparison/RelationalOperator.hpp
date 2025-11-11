// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_E4A7B3C9D2F1E8A6B5C4D3E2F1A0B9C8
#define WJH_ATLAS_E4A7B3C9D2F1E8A6B5C4D3E2F1A0B9C8

#include "atlas/generation/core/ITemplate.hpp"

namespace wjh::atlas::generation { inline namespace v1 {

/**
 * Relational operator template
 *
 * Generates relational comparison operators (<, >, <=, >=, ==, !=) that
 * delegate to the underlying type's comparison operators.
 *
 * This template is used when individual comparison operators are explicitly
 * requested in the StrongTypeDescription. When the spaceship operator (<=>)
 * is used instead, the SpaceshipOperator template handles all comparisons.
 *
 * The generated operators:
 * - Forward to underlying type's operators
 * - Preserve noexcept specification from underlying type
 * - Are marked constexpr when applicable
 * - Are defined as friend functions for symmetric behavior
 *
 * Performance characteristics:
 * - Zero-overhead wrapper around underlying type comparisons
 * - Noexcept specification forwarded from underlying type
 * - All operators are constexpr-capable when underlying type supports it
 *
 * Implementation note: This template overrides render_impl() to iterate
 * through all relational operators and render each one separately, similar
 * to how arithmetic operators are handled.
 */
class RelationalOperator final
: public ITemplate
{
protected:
    [[nodiscard]]
    std::string id_impl() const override
    {
        return "operators.comparison.relational";
    }

    /**
     * Sort key for relational operators
     *
     * Returns "!" to sort relational operators near the beginning of operator
     * sections, before most other operators. The "!" character sorts before
     * most operator symbols, ensuring comparison operators appear early.
     *
     * Note: This template iterates through multiple operators internally
     * (==, !=, <, <=, >, >=), but appears only once in the registry.
     *
     * @return Sort key: "!"
     */
    [[nodiscard]]
    std::string sort_key_impl() const override
    {
        return "!";
    }

    [[nodiscard]]
    std::string_view get_template_impl() const noexcept override;

    /**
     * Check if any relational operators are enabled
     *
     * Examines the ClassInfo to determine if any comparison
     * operators (<, >, <=, >=, ==, !=) have been requested.
     *
     * @param info Strong type class information
     * @return true if any relational operators are enabled
     */
    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;

    /**
     * Render all relational operators
     *
     * Overrides the default render_impl() to iterate through all
     * relational operators in ClassInfo::relational_operators and
     * render each one separately. This is necessary because the
     * template uses {{{op}}} which needs to be set for each operator.
     *
     * @param info Strong type class information
     * @return Rendered C++ code for all relational operators
     */
    [[nodiscard]]
    std::string render_impl(ClassInfo const & info) const override;

private:
    /**
     * Helper to prepare variables for a specific operator
     */
    [[nodiscard]]
    boost::json::object prepare_variables_for_operator(
        ClassInfo const & info,
        std::string_view op_symbol) const;
};

}} // namespace wjh::atlas::generation::v1

#endif // WJH_ATLAS_E4A7B3C9D2F1E8A6B5C4D3E2F1A0B9C8
