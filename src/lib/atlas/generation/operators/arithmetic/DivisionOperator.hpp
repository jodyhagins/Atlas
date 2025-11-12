// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_6A3F8D2E9C7B4F1D5A8E3C7B2F9D4A6E
#define WJH_ATLAS_6A3F8D2E9C7B4F1D5A8E3C7B2F9D4A6E

#include "atlas/generation/core/ITemplate.hpp"
#include "atlas/generation/parsing/OperatorParser.hpp"

namespace wjh::atlas::generation {

/**
 * Base class for division operator templates
 */
class DivisionOperatorBase
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
     * All division operator variants share the same operator symbol "/".
     *
     * @return Sort key: "/"
     */
    [[nodiscard]]
    std::string sort_key_impl() const override
    {
        return "/";
    }
};

class DefaultDivisionOperator final
: public DivisionOperatorBase
{
protected:
    [[nodiscard]]
    std::string id_impl() const override
    {
        return "operators.arithmetic.division.default";
    }

    [[nodiscard]]
    std::string_view get_template_impl() const noexcept override;

    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;
};

class CheckedDivisionOperator final
: public DivisionOperatorBase
{
protected:
    [[nodiscard]]
    std::string id_impl() const override
    {
        return "operators.arithmetic.division.checked";
    }

    [[nodiscard]]
    std::string_view get_template_impl() const noexcept override;

    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;
};

class SaturatingDivisionOperator final
: public DivisionOperatorBase
{
protected:
    [[nodiscard]]
    std::string id_impl() const override
    {
        return "operators.arithmetic.division.saturating";
    }

    [[nodiscard]]
    std::string_view get_template_impl() const noexcept override;

    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;
};

} // namespace wjh::atlas::generation

#endif // WJH_ATLAS_6A3F8D2E9C7B4F1D5A8E3C7B2F9D4A6E
