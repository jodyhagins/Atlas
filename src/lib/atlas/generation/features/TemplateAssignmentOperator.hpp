// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_E7B2F8C3D4A1E6F9B8C7D2E5F1A4B3C6
#define WJH_ATLAS_E7B2F8C3D4A1E6F9B8C7D2E5F1A4B3C6

#include "atlas/generation/core/ITemplate.hpp"

namespace wjh::atlas::generation { inline namespace v1 {

/**
 * Template assignment operator
 *
 * Generates a template assignment operator that allows assignment from any
 * type that is assignable to the underlying type:
 * - Uses SFINAE (C++11-17) or concepts (C++20+) to constrain the template
 * - Only accepts types that are assignable to the underlying type
 * - Rejects assignment from the strong type itself (avoids shadowing copy
 * assignment)
 * - Marked noexcept based on the underlying assignment's exception
 * specification
 *
 * This provides convenience while maintaining type safety:
 * - StrongType s{"initial"}; s = "new value"; // Works if assignable
 * - s = 42; // Rejected by SFINAE/concepts if not assignable
 *
 * Note: constexpr support varies by C++ standard:
 * - C++11: No constexpr (non-static member functions are implicitly const)
 * - C++14+: constexpr supported
 */
class TemplateAssignmentOperator final
: public ITemplate
{
protected:
    [[nodiscard]]
    std::string id_impl() const override
    {
        return "features.template_assignment";
    }

    [[nodiscard]]
    std::string_view get_template_impl() const noexcept override;

    /**
     * Check if template assignment operator is enabled
     *
     * Examines the ClassInfo to determine if the template
     * assignment operator feature has been requested.
     *
     * @param info Strong type class information
     * @return true if template assignment is enabled
     */
    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;

    /**
     * Prepare variables for template assignment operator rendering
     *
     * Creates a JSON object with variables needed for rendering:
     * - const_expr: "constexpr " or empty based on C++ standard
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

#endif // WJH_ATLAS_E7B2F8C3D4A1E6F9B8C7D2E5F1A4B3C6
