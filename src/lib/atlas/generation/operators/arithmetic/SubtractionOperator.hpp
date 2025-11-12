// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_3E7D9B2F6A8C4E1D5F7A9C3B8E2D6F4A
#define WJH_ATLAS_3E7D9B2F6A8C4E1D5F7A9C3B8E2D6F4A

#include "atlas/generation/core/ITemplate.hpp"
#include "atlas/generation/parsing/OperatorParser.hpp"

namespace wjh::atlas::generation { inline namespace v1 {

/**
 * Base class for subtraction operator templates
 *
 * Provides common should_apply logic for all subtraction operator modes.
 * Derived classes implement mode-specific behavior (Default, Checked,
 * Saturating, Wrapping).
 */
class SubtractionOperatorBase
: public ITemplate
{
protected:
    /**
     * Check if subtraction operator is enabled
     *
     * @param info Strong type class information
     * @return true if "-" operator is enabled
     */
    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;

    /**
     * Prepare common variables for subtraction operator rendering
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
     * All subtraction operator variants share the same operator symbol "-".
     *
     * @return Sort key: "-"
     */
    [[nodiscard]]
    std::string sort_key_impl() const override
    {
        return "-";
    }
};

/**
 * Default subtraction operator template
 *
 * Generates the standard subtraction operator using operator-= forwarding.
 */
class DefaultSubtractionOperator final
: public SubtractionOperatorBase
{
protected:
    [[nodiscard]]
    std::string id_impl() const override
    {
        return "operators.arithmetic.subtraction.default";
    }

    [[nodiscard]]
    std::string_view get_template_impl() const noexcept override;

    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;
};

/**
 * Checked subtraction operator template
 *
 * Generates subtraction with overflow/underflow detection.
 */
class CheckedSubtractionOperator final
: public SubtractionOperatorBase
{
protected:
    [[nodiscard]]
    std::string id_impl() const override
    {
        return "operators.arithmetic.subtraction.checked";
    }

    [[nodiscard]]
    std::string_view get_template_impl() const noexcept override;

    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;
};

/**
 * Saturating subtraction operator template
 *
 * Generates subtraction that clamps to type limits.
 */
class SaturatingSubtractionOperator final
: public SubtractionOperatorBase
{
protected:
    [[nodiscard]]
    std::string id_impl() const override
    {
        return "operators.arithmetic.subtraction.saturating";
    }

    [[nodiscard]]
    std::string_view get_template_impl() const noexcept override;

    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;
};

/**
 * Wrapping subtraction operator template
 *
 * Generates subtraction with well-defined overflow behavior.
 */
class WrappingSubtractionOperator final
: public SubtractionOperatorBase
{
protected:
    [[nodiscard]]
    std::string id_impl() const override
    {
        return "operators.arithmetic.subtraction.wrapping";
    }

    [[nodiscard]]
    std::string_view get_template_impl() const noexcept override;

    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;
};

}} // namespace wjh::atlas::generation::v1

#endif // WJH_ATLAS_3E7D9B2F6A8C4E1D5F7A9C3B8E2D6F4A
