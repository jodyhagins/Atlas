// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_B4C8D1E5F2A7B9C6D4E8F3A1B5C9D7E2
#define WJH_ATLAS_B4C8D1E5F2A7B9C6D4E8F3A1B5C9D7E2

#include "atlas/generation/core/ITemplate.hpp"

namespace wjh::atlas::generation { inline namespace v1 {

/**
 * Indirection operator template
 *
 * Generates the dereference operator (*) that forwards to the wrapped type:
 * - For pointer types: dereferences the pointer (returns *ptr)
 * - For pointer-like types (smart pointers, iterators, optional): returns
 * *value
 * - For other types: returns reference to value (fallback)
 *
 * The generated operator provides both const and non-const overloads using
 * template parameters to enable proper const forwarding.
 *
 * Performance characteristics:
 * - Zero-overhead forwarding to underlying type
 * - Uses SFINAE with PriorityTag to select correct implementation
 * - Marked constexpr when applicable
 *
 * Design note: This operator uses atlas::atlas_detail::star_impl which
 * handles the complexity of detecting pointer vs pointer-like vs regular types.
 * Note that the const and non-const versions use different PriorityTag values
 * (1 vs 10) to ensure proper overload resolution.
 */
class IndirectionOperator final
: public ITemplate
{
protected:
    [[nodiscard]]
    std::string id_impl() const override
    {
        return "operators.access.indirection";
    }

    /**
     * Sort key for indirection operator
     *
     * @return Sort key: "*" (the indirection/dereference operator symbol)
     */
    [[nodiscard]]
    std::string sort_key_impl() const override
    {
        return "*";
    }

    [[nodiscard]]
    std::string_view get_template_impl() const noexcept override;

    /**
     * Check if indirection operator is enabled
     *
     * Examines the ClassInfo to determine if the "*" operator
     * has been requested for this strong type.
     *
     * @param info Strong type class information
     * @return true if "*" operator is enabled
     */
    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;

    /**
     * Prepare variables for indirection operator rendering
     *
     * Creates a JSON object with variables needed for rendering:
     * - const_expr: "constexpr" or empty string based on settings
     *
     * @param info Strong type class information
     * @return JSON object with template variables
     */
    [[nodiscard]]
    boost::json::object prepare_variables_impl(
        ClassInfo const & info) const override;
};

}} // namespace wjh::atlas::generation::v1

#endif // WJH_ATLAS_B4C8D1E5F2A7B9C6D4E8F3A1B5C9D7E2
