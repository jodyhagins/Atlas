// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_8D6F3A2E9C7B5F4D1A8E6C3B2F9D7A4E
#define WJH_ATLAS_8D6F3A2E9C7B5F4D1A8E6C3B2F9D7A4E

#include "atlas/generation/core/ITemplate.hpp"

namespace wjh::atlas::generation { inline namespace v1 {

/**
 * Template for increment/decrement operators (++, --)
 *
 * Generates both prefix and postfix variants:
 * - Prefix: ++x and --x (modifies and returns reference)
 * - Postfix: x++ and x-- (modifies, returns old value)
 *
 * Unlike arithmetic binary operators, increment/decrement operators
 * only have one mode (Default) - there are no checked/saturating/wrapping
 * variants in the current implementation.
 *
 * Performance:
 * - Single parse per method call
 * - Static template string (zero-cost)
 * - noexcept where possible
 */
class IncrementOperatorsTemplate final
: public ITemplate
{
protected:
    [[nodiscard]]
    std::string id_impl() const override
    {
        return "operators.arithmetic.increment";
    }

    /**
     * Sort key for increment operators
     *
     * Returns "++" to sort increment operators with other arithmetic
     * operators. Uses "++" since it's the primary increment operator
     * (both ++ and -- are handled by this template).
     *
     * @return Sort key: "++"
     */
    [[nodiscard]]
    std::string sort_key_impl() const override
    {
        return "++";
    }

    [[nodiscard]]
    std::string_view get_template_impl() const noexcept override;

    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;

    [[nodiscard]]
    boost::json::object prepare_variables_impl(
        ClassInfo const & info) const override;

    /**
     * Custom render implementation that loops over all increment operators
     *
     * Unlike templates that pre-accumulate operators (like arithmetic binary),
     * increment operators are rendered individually for each operator (++, --)
     * and accumulated into a single output string.
     *
     * @param info Strong type class information
     * @return Rendered C++ code for all increment operators
     */
    [[nodiscard]]
    std::string render_impl(ClassInfo const & info) const override;

    /**
     * Prepare variables for a specific operator
     *
     * Helper method that adds the operator symbol to the base variables.
     *
     * @param info Strong type class information
     * @param op_symbol The operator symbol ("++", "--")
     * @return JSON object with variables including the operator
     */
    [[nodiscard]]
    boost::json::object prepare_variables_for_operator(
        ClassInfo const & info,
        std::string_view op_symbol) const;
};

}} // namespace wjh::atlas::generation::v1

#endif // WJH_ATLAS_8D6F3A2E9C7B5F4D1A8E6C3B2F9D7A4E
