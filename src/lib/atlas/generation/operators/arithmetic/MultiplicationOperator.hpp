// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_2D8F7A3E9C6B4F1D5A8E7C2B9F4D3A6E
#define WJH_ATLAS_2D8F7A3E9C6B4F1D5A8E7C2B9F4D3A6E

#include "atlas/generation/core/ITemplate.hpp"
#include "atlas/generation/parsing/OperatorParser.hpp"

namespace wjh::atlas::generation { inline namespace v1 {

/**
 * Base class for multiplication operator templates
 *
 * Provides common should_apply logic for all multiplication operator modes.
 * Performance considerations:
 * - Template strings are static class members (zero-cost)
 * - All non-throwing methods are marked noexcept
 *
 * TODO(performance): Current implementation parses ClassInfo twice per
 * applicable template (once in should_apply_impl, once in
 * prepare_variables_impl). Consider caching parse results or restructuring
 * ITemplate to accept pre-parsed ClassInfo. Estimated 2x overhead during code
 * generation.
 */
class MultiplicationOperatorBase
: public ITemplate
{
protected:
    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;

    [[nodiscard]]
    boost::json::object prepare_variables_impl(
        ClassInfo const & info) const override;

    /**
     * Return operator symbol for sorting
     *
     * All multiplication operator variants share the same operator symbol "*".
     *
     * @return Sort key: "*"
     */
    [[nodiscard]]
    std::string sort_key_impl() const override
    {
        return "*";
    }
};

/**
 * Default multiplication operator template
 */
class DefaultMultiplicationOperator final
: public MultiplicationOperatorBase
{
protected:
    [[nodiscard]]
    std::string id_impl() const override
    {
        return "operators.arithmetic.multiplication.default";
    }

    [[nodiscard]]
    std::string_view get_template_impl() const noexcept override;

    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;
};

/**
 * Checked multiplication operator template
 */
class CheckedMultiplicationOperator final
: public MultiplicationOperatorBase
{
protected:
    [[nodiscard]]
    std::string id_impl() const override
    {
        return "operators.arithmetic.multiplication.checked";
    }

    [[nodiscard]]
    std::string_view get_template_impl() const noexcept override;

    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;
};

/**
 * Saturating multiplication operator template
 */
class SaturatingMultiplicationOperator final
: public MultiplicationOperatorBase
{
protected:
    [[nodiscard]]
    std::string id_impl() const override
    {
        return "operators.arithmetic.multiplication.saturating";
    }

    [[nodiscard]]
    std::string_view get_template_impl() const noexcept override;

    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;
};

/**
 * Wrapping multiplication operator template
 */
class WrappingMultiplicationOperator final
: public MultiplicationOperatorBase
{
protected:
    [[nodiscard]]
    std::string id_impl() const override
    {
        return "operators.arithmetic.multiplication.wrapping";
    }

    [[nodiscard]]
    std::string_view get_template_impl() const noexcept override;

    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;
};

}} // namespace wjh::atlas::generation::v1

#endif // WJH_ATLAS_2D8F7A3E9C6B4F1D5A8E7C2B9F4D3A6E
