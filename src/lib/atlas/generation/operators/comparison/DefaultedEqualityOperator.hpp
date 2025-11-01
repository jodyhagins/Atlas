// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_A9E2F1B8C3D7E4F6A2B9C5D1E8F3A7B4
#define WJH_ATLAS_A9E2F1B8C3D7E4F6A2B9C5D1E8F3A7B4

#include "atlas/generation/core/ITemplate.hpp"

namespace wjh::atlas::generation { inline namespace v1 {

/**
 * Defaulted equality operator (==) template
 *
 * Generates the equality comparison operator with C++17 fallback.
 * This template is typically used in conjunction with SpaceshipOperator
 * to provide a complete set of comparison operations.
 *
 * In C++20 mode:
 * - Generates defaulted operator== which the compiler can optimize
 * - The compiler automatically generates operator!= from operator==
 * - Provides optimal performance with minimal code generation
 *
 * In C++17 fallback mode:
 * - Manually generates both operator== and operator!=
 * - Each operator delegates to the underlying type's comparison
 * - Provides equivalent functionality to C++20's synthesized operators
 *
 * Design rationale:
 * When the spaceship operator is present, it's beneficial to provide
 * a separate defaulted equality operator for two reasons:
 * 1. Performance: Equality checks are often faster than three-way comparison
 * 2. Semantics: Not all types with ordering support equality efficiently
 *
 * The defaulted equality operator is automatically added when:
 * - Spaceship operator is requested alone (no other comparison operators)
 * - Spaceship operator is requested with explicit equality operators (==, !=)
 *
 * Performance characteristics:
 * - C++20: Compiler-optimized defaulted operator
 * - C++17: Zero-overhead delegation to underlying type
 * - All operators preserve noexcept specification from underlying type
 * - All operators are constexpr-capable when underlying type supports it
 */
class DefaultedEqualityOperator final
: public ITemplate
{
protected:
    [[nodiscard]]
    std::string id_impl() const override
    {
        return "operators.comparison.defaulted_equality";
    }

    [[nodiscard]]
    std::string_view get_template_impl() const noexcept override;

    /**
     * Check if defaulted equality operator should be generated
     *
     * Examines the ClassInfo to determine if the defaulted
     * equality operator has been enabled. This is typically set when:
     * - Spaceship operator is alone (auto-generated defaulted ==)
     * - Spaceship operator is with == or != (use defaulted instead of custom)
     *
     * @param info Strong type class information
     * @return true if defaulted equality operator should be generated
     */
    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;

    /**
     * Prepare variables for defaulted equality operator rendering
     *
     * Creates a JSON object with variables needed for rendering:
     * - class_name
     * - underlying_type
     * - const_expr
     *
     * @param info Strong type class information
     * @return JSON object with template variables
     */
    [[nodiscard]]
    boost::json::object prepare_variables_impl(
        ClassInfo const & info) const override;
};

}} // namespace wjh::atlas::generation::v1

#endif // WJH_ATLAS_A9E2F1B8C3D7E4F6A2B9C5D1E8F3A7B4
