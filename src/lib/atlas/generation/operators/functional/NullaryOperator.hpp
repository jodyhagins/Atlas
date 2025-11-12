// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_3F9E8D7C6B5A4D3E2F1A0B9C8D7E6F5B
#define WJH_ATLAS_3F9E8D7C6B5A4D3E2F1A0B9C8D7E6F5B

#include "atlas/generation/core/ITemplate.hpp"

namespace wjh::atlas::generation { inline namespace v1 {

/**
 * Nullary call operator template
 *
 * Generates a nullary call operator (operator()) for a strong type:
 * - Returns a reference to the wrapped value
 * - Provides both const and non-const overloads
 * - Both overloads are noexcept
 *
 * The generated operator allows strong types to be invoked like a function
 * with no arguments to access the underlying value.
 *
 * Example:
 * @code
 * UserId id{42};
 * int value = id();  // Returns: 42
 * @endcode
 *
 * Design notes:
 * - The operator returns a direct reference to the underlying value
 * - No copies are made, enabling efficient access
 * - Both overloads are unconditionally noexcept
 */
class NullaryOperator final
: public ITemplate
{
protected:
    [[nodiscard]]
    std::string id_impl() const override
    {
        return "operators.functional.nullary";
    }

    [[nodiscard]]
    std::string_view get_template_impl() const noexcept override;

    /**
     * Check if nullary call operator is enabled
     *
     * Examines the ClassInfo to determine if the "()" operator
     * has been requested for this strong type.
     *
     * @param info Strong type class information
     * @return true if nullary operator is enabled
     */
    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;
};

}} // namespace wjh::atlas::generation::v1

#endif // WJH_ATLAS_3F9E8D7C6B5A4D3E2F1A0B9C8D7E6F5B
