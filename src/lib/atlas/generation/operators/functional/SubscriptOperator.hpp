// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_5F9E8D7C6B5A4D3E2F1A0B9C8D7E6F5D
#define WJH_ATLAS_5F9E8D7C6B5A4D3E2F1A0B9C8D7E6F5D

#include "atlas/generation/core/ITemplate.hpp"

namespace wjh::atlas::generation { inline namespace v1 {

/**
 * Subscript operator template
 *
 * Generates subscript operator (operator[]) for a strong type:
 * - Forwards subscript operations to the wrapped value
 * - Supports single argument (C++17) and multidimensional subscript (C++23+)
 * - Returns decltype(auto) to preserve return type and value category
 * - Provides both const and non-const overloads
 * - Conditionally noexcept based on the underlying type's operator[]
 * - Uses perfect forwarding for arguments
 *
 * The generated operator allows strong types wrapping containers or
 * array-like types to be indexed naturally.
 *
 * Example:
 * @code
 * StringVec vec{std::vector<std::string>{"a", "b", "c"}};
 * std::string& first = vec[0];  // Returns: "a"
 *
 * // C++23 multidimensional subscript:
 * Matrix2D mat{...};
 * auto& elem = mat[i, j];
 * @endcode
 *
 * Design notes:
 * - Uses __cpp_multidimensional_subscript >= 202110L to detect C++23 support
 * - Falls back to single-argument subscript for earlier standards
 * - Perfect forwarding preserves value category of arguments
 * - Noexcept specification propagates from the underlying type
 * - Return type uses decltype(auto) to avoid copies and preserve references
 */
class SubscriptOperator final
: public ITemplate
{
protected:
    [[nodiscard]]
    std::string id_impl() const override
    {
        return "operators.functional.subscript";
    }

    [[nodiscard]]
    std::string_view get_template_impl() const noexcept override;

    /**
     * Check if subscript operator is enabled
     *
     * Examines the ClassInfo to determine if the "[]" operator
     * has been requested for this strong type.
     *
     * @param info Strong type class information
     * @return true if subscript operator is enabled
     */
    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;
};

}} // namespace wjh::atlas::generation::v1

#endif // WJH_ATLAS_5F9E8D7C6B5A4D3E2F1A0B9C8D7E6F5D
