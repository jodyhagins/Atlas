// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_D6C9E8F7A2B3D4E1F8C5D9E2F7A1B4C8
#define WJH_ATLAS_D6C9E8F7A2B3D4E1F8C5D9E2F7A1B4C8

#include "atlas/generation/core/ITemplate.hpp"

namespace wjh::atlas::generation { inline namespace v1 {

/**
 * Iterator support template
 *
 * Generates container-like interface members for strong types wrapping
 * containers:
 * - Type aliases: iterator, const_iterator, value_type
 * - Member functions: begin(), end() with const and non-const overloads
 * - Uses ADL-enabled helpers (atlas::atlas_detail::begin_, end_)
 * - Enables range-based for loops: for (auto& x : strong_container) { }
 *
 * The generated interface:
 * - Deduces iterator types from the underlying type
 * - Preserves noexcept specifications from the underlying begin/end
 * - Supports both explicit calls (s.begin()) and range-based for loops
 * - Works with any type that has begin()/end() (via ADL or std::begin/end)
 *
 * Design philosophy:
 * - Zero-overhead abstraction over container iteration
 * - Compile-time type deduction using decltype
 * - Perfect noexcept propagation
 */
class IteratorSupportTemplate final
: public ITemplate
{
protected:
    [[nodiscard]]
    std::string id_impl() const override
    {
        return "features.iterator_support";
    }

    [[nodiscard]]
    std::string_view get_template_impl() const noexcept override;

    /**
     * Check if iterator support is enabled
     *
     * Examines the ClassInfo to determine if iterator support
     * has been requested for this strong type.
     *
     * @param info Strong type class information
     * @return true if iterator support is enabled
     */
    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;

    /**
     * Prepare variables for iterator support rendering
     *
     * Creates a JSON object with variables needed for rendering:
     * - const_expr: "constexpr " or empty based on C++ standard
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

#endif // WJH_ATLAS_D6C9E8F7A2B3D4E1F8C5D9E2F7A1B4C8
