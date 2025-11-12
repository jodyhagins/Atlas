// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_9B4F7D3E2A8C6F1D5E7A4C9B3F8D2A6E
#define WJH_ATLAS_9B4F7D3E2A8C6F1D5E7A4C9B3F8D2A6E

#include "atlas/generation/core/ITemplate.hpp"
#include "atlas/generation/parsing/OperatorParser.hpp"

namespace wjh::atlas::generation {

/**
 * Base class for modulo operator templates
 */
class ModuloOperatorBase
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
     * All modulo operator variants share the same operator symbol "%".
     *
     * @return Sort key: "%"
     */
    [[nodiscard]]
    std::string sort_key_impl() const override
    {
        return "%";
    }
};

class DefaultModuloOperator final
: public ModuloOperatorBase
{
protected:
    [[nodiscard]]
    std::string id_impl() const override
    {
        return "operators.arithmetic.modulo.default";
    }

    [[nodiscard]]
    std::string_view get_template_impl() const noexcept override;

    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;
};

class CheckedModuloOperator final
: public ModuloOperatorBase
{
protected:
    [[nodiscard]]
    std::string id_impl() const override
    {
        return "operators.arithmetic.modulo.checked";
    }

    [[nodiscard]]
    std::string_view get_template_impl() const noexcept override;

    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;
};

class SaturatingModuloOperator final
: public ModuloOperatorBase
{
protected:
    [[nodiscard]]
    std::string id_impl() const override
    {
        return "operators.arithmetic.modulo.saturating";
    }

    [[nodiscard]]
    std::string_view get_template_impl() const noexcept override;

    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;
};

} // namespace wjh::atlas::generation

#endif // WJH_ATLAS_9B4F7D3E2A8C6F1D5E7A4C9B3F8D2A6E
