// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_D8F3E1A6C5B4D9E2F7A3B1C8D6E9F2A5
#define WJH_ATLAS_D8F3E1A6C5B4D9E2F7A3B1C8D6E9F2A5

#include "atlas/generation/core/ITemplate.hpp"

namespace wjh::atlas::generation {

/**
 * Spaceship operator (<=>) template
 *
 * Generates the C++20 three-way comparison operator (<=>) with C++17 fallback.
 *
 * In C++20 mode:
 * - Generates defaulted operator<=> which synthesizes all comparison operators
 * - The compiler automatically generates <, <=, >, >= from operator<=>
 *
 * In C++17 fallback mode:
 * - Manually generates all four relational operators (<, <=, >, >=)
 * - Each operator delegates to the underlying type's comparison
 * - Provides equivalent functionality to C++20's synthesized operators
 *
 * The spaceship operator provides total ordering for types that support it.
 * When combined with DefaultedEqualityOperator, it provides a complete set
 * of comparison operations.
 *
 * Performance characteristics:
 * - C++20: Single defaulted operator, compiler-optimized
 * - C++17: Four manual operators, but still zero-overhead delegation
 * - All operators preserve noexcept specification from underlying type
 * - All operators are constexpr-capable when underlying type supports it
 *
 * Design notes:
 * - Requires <compare> header in C++20 mode
 * - Uses feature-test macro to detect three-way comparison support
 * - C++17 fallback ensures backward compatibility
 * - Works with any underlying type that supports comparison operators
 */
class SpaceshipOperator final
: public ITemplate
{
protected:
    [[nodiscard]]
    std::string id_impl() const override
    {
        return "operators.comparison.spaceship";
    }

    [[nodiscard]]
    std::string_view get_template_impl() const noexcept override;

    /**
     * Check if spaceship operator is enabled
     *
     * Examines the ClassInfo to determine if the "<=>"
     * operator has been requested for this strong type.
     *
     * @param info Strong type class information
     * @return true if "<=>" operator is enabled
     */
    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;

    /**
     * Required includes for spaceship operator
     *
     * In C++20 mode, the spaceship operator requires <compare> header.
     * The C++17 fallback uses no additional headers.
     *
     * @return Set containing "<compare>" for C++20 support
     */
    [[nodiscard]]
    std::set<std::string> required_includes_impl() const override;
};

} // namespace wjh::atlas::generation

#endif // WJH_ATLAS_D8F3E1A6C5B4D9E2F7A3B1C8D6E9F2A5
