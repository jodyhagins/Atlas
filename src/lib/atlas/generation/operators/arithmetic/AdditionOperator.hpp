// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_4F8E2A9D7C6B3E5F1A4D8C2E6F9B3A7D
#define WJH_ATLAS_4F8E2A9D7C6B3E5F1A4D8C2E6F9B3A7D

#include "atlas/generation/core/ITemplate.hpp"
#include "atlas/generation/parsing/OperatorParser.hpp"

namespace wjh::atlas::generation { inline namespace v1 {

/**
 * Base class for addition operator templates
 *
 * Provides common should_apply logic for all addition operator modes.
 * Derived classes implement mode-specific behavior (Default, Checked,
 * Saturating, Wrapping).
 */
class AdditionOperatorBase
: public ITemplate
{
protected:
    /**
     * Check if addition operator is enabled
     *
     * Examines the ClassInfo to determine if the "+" operator
     * has been requested for this strong type.
     *
     * @param info Strong type class information
     * @return true if "+" operator is enabled
     */
    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;

    /**
     * Prepare common variables for addition operator rendering
     *
     * Creates a JSON object with variables common to all addition modes:
     * - class_name
     * - underlying_type
     * - full_qualified_name
     * - has_constraint
     * - constraint_message
     * - op: "+"
     * - const_expr
     *
     * @param info Strong type class information
     * @return JSON object with template variables
     */
    [[nodiscard]]
    boost::json::object prepare_variables_impl(
        ClassInfo const & info) const override;

    /**
     * Return operator symbol for sorting
     *
     * All addition operator variants (default, checked, saturating, wrapping)
     * share the same operator symbol "+", ensuring they sort together.
     *
     * @return Sort key: "+"
     */
    [[nodiscard]]
    std::string sort_key_impl() const override
    {
        return "+";
    }
};

/**
 * Default addition operator template
 *
 * Generates the standard addition operator using operator+= forwarding:
 * - friend operator+= (modifies lhs, returns lhs&)
 * - friend operator+ (uses +=, returns by value)
 *
 * This is the standard idiom for arithmetic operators and works for
 * all types that support +=.
 *
 * Template rendering:
 * - Generates both += and + operators
 * - Uses perfect noexcept forwarding from underlying type
 * - Validates constraints if present
 * - Marked constexpr when appropriate
 */
class DefaultAdditionOperator final
: public AdditionOperatorBase
{
protected:
    [[nodiscard]]
    std::string id_impl() const override
    {
        return "operators.arithmetic.addition.default";
    }

    [[nodiscard]]
    std::string_view get_template_impl() const noexcept override;

    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;
};

/**
 * Checked addition operator template
 *
 * Generates addition with overflow/underflow detection:
 * - Uses atlas::atlas_detail::checked_add()
 * - Throws CheckedOverflowError on overflow
 * - Throws CheckedUnderflowError on underflow (signed types only)
 * - Still validates constraints if present
 *
 * Performance characteristics:
 * - Additional overflow checks at runtime
 * - Exception-based error handling
 * - Cannot be noexcept or constexpr
 */
class CheckedAdditionOperator final
: public AdditionOperatorBase
{
protected:
    [[nodiscard]]
    std::string id_impl() const override
    {
        return "operators.arithmetic.addition.checked";
    }

    [[nodiscard]]
    std::string_view get_template_impl() const noexcept override;

    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;
};

/**
 * Saturating addition operator template
 *
 * Generates addition that clamps to type limits:
 * - Uses atlas::atlas_detail::saturating_add()
 * - Overflow/underflow clamps to std::numeric_limits instead of throwing
 * - Marked noexcept (unless constraints present)
 * - Still validates constraints if present
 *
 * Use cases:
 * - Graphics/audio processing (clamping is desired behavior)
 * - Systems where exceptions are unacceptable
 * - Algorithms that naturally saturate at boundaries
 */
class SaturatingAdditionOperator final
: public AdditionOperatorBase
{
protected:
    [[nodiscard]]
    std::string id_impl() const override
    {
        return "operators.arithmetic.addition.saturating";
    }

    [[nodiscard]]
    std::string_view get_template_impl() const noexcept override;

    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;
};

/**
 * Wrapping addition operator template
 *
 * Generates addition with well-defined overflow behavior:
 * - Uses unsigned arithmetic to avoid UB on signed overflow
 * - Overflow wraps around (2's complement behavior)
 * - Marked noexcept (unless constraints present)
 * - Only available for integral types (enforced via static_assert)
 * - Still validates constraints if present
 *
 * Use cases:
 * - Cryptographic operations
 * - Hash functions
 * - Modular arithmetic algorithms
 * - Systems where wraparound is the desired behavior
 */
class WrappingAdditionOperator final
: public AdditionOperatorBase
{
protected:
    [[nodiscard]]
    std::string id_impl() const override
    {
        return "operators.arithmetic.addition.wrapping";
    }

    [[nodiscard]]
    std::string_view get_template_impl() const noexcept override;

    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;
};

}} // namespace wjh::atlas::generation::v1

#endif // WJH_ATLAS_4F8E2A9D7C6B3E5F1A4D8C2E6F9B3A7D
