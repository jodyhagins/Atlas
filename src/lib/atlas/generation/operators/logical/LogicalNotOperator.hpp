// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_B4D8E1F9A7C2D6E3F8B1C9D4E7F2A5B8
#define WJH_ATLAS_B4D8E1F9A7C2D6E3F8B1C9D4E7F2A5B8

#include "atlas/generation/core/ITemplate.hpp"

namespace wjh::atlas::generation { inline namespace v1 {

/**
 * Logical NOT operator template
 *
 * Generates the unary logical NOT operator (!) for a strong type:
 * - Returns bool result of applying ! to the wrapped value
 * - Marked noexcept if the underlying operation is noexcept
 * - Uses friend function for proper ADL and symmetry
 *
 * The generated operator follows standard C++ semantics for logical NOT,
 * converting the wrapped value to bool and negating it.
 *
 * Note: Uses "not" keyword form in generated code for consistency with
 * modern C++ style guidelines.
 */
class LogicalNotOperator final
: public ITemplate
{
protected:
    [[nodiscard]]
    std::string id_impl() const override
    {
        return "operators.logical.not";
    }

    /**
     * Sort key for logical NOT operator
     *
     * @return Sort key: "!" (the logical NOT operator symbol)
     */
    [[nodiscard]]
    std::string sort_key_impl() const override
    {
        return "!";
    }

    [[nodiscard]]
    std::string_view get_template_impl() const noexcept override;

    /**
     * Check if logical NOT operator is enabled
     *
     * Examines the ClassInfo to determine if the "!" or "not" operator
     * has been requested for this strong type.
     *
     * @param info Strong type class information
     * @return true if "!" operator is enabled
     */
    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;

    /**
     * Prepare variables for logical NOT operator rendering
     *
     * Creates a JSON object with variables needed for rendering:
     * - const_expr: "constexpr" or empty string based on settings
     * - class_name: name of the strong type class
     * - underlying_type: the wrapped type
     *
     * @param info Strong type class information
     * @return JSON object with template variables
     */
    [[nodiscard]]
    boost::json::object prepare_variables_impl(
        ClassInfo const & info) const override;
};

}} // namespace wjh::atlas::generation::v1

#endif // WJH_ATLAS_B4D8E1F9A7C2D6E3F8B1C9D4E7F2A5B8
