// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_7C5F2D8E4A9B6F3D1E8A5C2B7F4D9A3E
#define WJH_ATLAS_7C5F2D8E4A9B6F3D1E8A5C2B7F4D9A3E

#include "atlas/generation/core/ITemplate.hpp"

namespace wjh::atlas::generation { inline namespace v1 {

/**
 * Template for unary operators (+, -, ~)
 *
 * Generates unary operator implementations that:
 * - Apply the operator to the wrapped value
 * - Return a new instance with the modified value
 * - Are marked constexpr when appropriate
 * - Have noexcept specification based on underlying type
 *
 * Unlike arithmetic binary operators, unary operators only have
 * one mode (Default) - there are no checked/saturating/wrapping variants.
 */
class UnaryOperatorsTemplate final
: public ITemplate
{
protected:
    [[nodiscard]]
    std::string id_impl() const override
    {
        return "operators.arithmetic.unary";
    }

    /**
     * Sort key for unary operators
     *
     * Returns "+" to sort unary operators with arithmetic operators.
     * The unary "+" is chosen as it's the most basic unary operator
     * (this template handles +, -, and ~ operators).
     *
     * @return Sort key: "+"
     */
    [[nodiscard]]
    std::string sort_key_impl() const override
    {
        return "+";
    }

    [[nodiscard]]
    std::string_view get_template_impl() const noexcept override;

    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;

    /**
     * Custom render implementation that loops over all unary operators
     *
     * Like increment operators, unary operators are rendered individually
     * for each operator (+, -, ~) and accumulated into a single output string.
     *
     * @param info Strong type class information
     * @return Rendered C++ code for all unary operators
     */
    [[nodiscard]]
    std::string render_impl(ClassInfo const & info) const override;

    /**
     * Prepare variables for a specific operator
     *
     * Helper that adds the operator symbol to the base variables.
     *
     * @param info Strong type class information
     * @param op_symbol The operator symbol ("+", "-", "~")
     * @return JSON object with variables including the operator
     */
    [[nodiscard]]
    boost::json::object prepare_variables_for_operator(
        ClassInfo const & info,
        std::string_view op_symbol) const;
};

}} // namespace wjh::atlas::generation::v1

#endif // WJH_ATLAS_7C5F2D8E4A9B6F3D1E8A5C2B7F4D9A3E
