// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_A8F3E9B7C2D6E4F1A9B8C5D7E3F2A6B1
#define WJH_ATLAS_A8F3E9B7C2D6E4F1A9B8C5D7E3F2A6B1

#include "atlas/generation/core/ITemplate.hpp"

namespace wjh::atlas::generation { inline namespace v1 {

// Forward declaration
struct ForwardedMemfn;

/**
 * Forwarded member function template
 *
 * Generates member functions that forward calls to the underlying value:
 * - Perfect forwarding of arguments
 * - Preserves const-correctness with multiple overloads
 * - Optional aliasing: forward size() as length()
 * - Optional return type transformation: wrap return in strong type
 * - C++23 deducing this support (single elegant overload)
 * - C++11-20 ref-qualified overloads (const &, const &&, &, &&)
 * - Constraint checking support for operations that may violate constraints
 *
 * Each forwarded member function generates up to 5 overloads (or 1 in C++23+):
 * 1. const & - for const lvalue objects
 * 2. const && - for const rvalue objects
 * 3. & - for non-const lvalue objects
 * 4. && - for non-const rvalue objects
 * 5. C++23: deducing this (replaces all 4 above)
 *
 * If const_only is true, only const overloads are generated.
 *
 * This template is unusual in that it's applied per-forwarded-function rather
 * than once per class. It's applied multiple times if multiple functions are
 * forwarded.
 *
 * Implementation note: This template overrides render_impl() to iterate
 * through all forwarded member functions and render each one separately,
 * similar to how relational operators are handled.
 */
class ForwardedMemfnTemplate final
: public ITemplate
{
protected:
    [[nodiscard]]
    std::string id_impl() const override
    {
        return "features.forwarded_memfn";
    }

    [[nodiscard]]
    std::string_view get_template_impl() const noexcept override;

    /**
     * Check if forwarded member functions are defined
     *
     * Examines the ClassInfo to determine if any member functions
     * should be forwarded to the underlying type.
     *
     * @param info Strong type class information
     * @return true if forwarded_memfns vector is non-empty
     */
    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;

    /**
     * Prepare variables for forwarded member function rendering
     *
     * Creates a JSON object with variables needed for rendering a single
     * forwarded member function:
     * - const_expr: "constexpr " or empty
     * - class_name: name of the strong type (for constraint messages)
     * - has_constraint: true if type has constraints
     * - memfn_name: original function name
     * - alias_name: alternate name (optional)
     * - return_type: wrapping type for return value (optional)
     * - const_only: if true, only const overloads
     * - generate_const_no_ref, generate_const_lvalue, etc.: control which
     * overloads
     *
     * This method is still required by the interface, but it's not used
     * since we override render_impl(). For completeness, return base variables
     * without the per-function fields.
     *
     * @param info Strong type class information
     * @return JSON object with template variables
     */
    [[nodiscard]]
    boost::json::object prepare_variables_impl(
        ClassInfo const & info) const override;

    /**
     * Render all forwarded member functions
     *
     * Overrides the default render_impl() to iterate through all
     * forwarded member functions in ClassInfo::forwarded_memfns and
     * render each one separately. This is necessary because the
     * template uses {{memfn_name}}, {{alias_name}}, etc. which need
     * to be set for each function.
     *
     * @param info Strong type class information
     * @return Rendered C++ code for all forwarded member functions
     */
    [[nodiscard]]
    std::string render_impl(ClassInfo const & info) const override;

private:
    /**
     * Helper to prepare variables for a specific forwarded member function
     *
     * @param info Strong type class information
     * @param fwd The specific forwarded member function to render
     * @return JSON object with template variables for this function
     */
    [[nodiscard]]
    boost::json::object prepare_variables_for_function(
        ClassInfo const & info,
        ForwardedMemfn const & fwd) const;
};

}} // namespace wjh::atlas::generation::v1

#endif // WJH_ATLAS_A8F3E9B7C2D6E4F1A9B8C5D7E3F2A6B1
