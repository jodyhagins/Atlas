// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_A3B7C9D2E4F1A6B8C5D3E7F2A9B1C4D6
#define WJH_ATLAS_A3B7C9D2E4F1A6B8C5D3E7F2A9B1C4D6

#include "atlas/generation/core/ITemplate.hpp"

namespace wjh::atlas::generation {

/**
 * Arrow operator template
 *
 * Generates the arrow operator (->) that forwards to the wrapped type:
 * - For pointer types: returns the pointer itself (built-in -> handles the
 * rest)
 * - For pointer-like types (smart pointers): returns value.operator->()
 * - For other types: returns &value
 *
 * The generated operator provides both const and non-const overloads using
 * template parameters to enable proper const forwarding.
 *
 * Performance characteristics:
 * - Zero-overhead forwarding to underlying type
 * - Uses SFINAE with PriorityTag to select correct implementation
 * - Marked constexpr when applicable
 *
 * Design note: This operator uses atlas::atlas_detail::arrow_impl which
 * handles the complexity of detecting pointer vs pointer-like vs regular types.
 */
class ArrowOperator final
: public ITemplate
{
protected:
    [[nodiscard]]
    std::string id_impl() const override
    {
        return "operators.access.arrow";
    }

    /**
     * Sort key for arrow operator
     *
     * @return Sort key: "->" (the arrow operator symbol)
     */
    [[nodiscard]]
    std::string sort_key_impl() const override
    {
        return "->";
    }

    [[nodiscard]]
    std::string_view get_template_impl() const noexcept override;

    /**
     * Check if arrow operator is enabled
     *
     * Examines the ClassInfo to determine if the "->" operator
     * has been requested for this strong type.
     *
     * @param info Strong type class information
     * @return true if "->" operator is enabled
     */
    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;
};

} // namespace wjh::atlas::generation

#endif // WJH_ATLAS_A3B7C9D2E4F1A6B8C5D3E7F2A9B1C4D6
