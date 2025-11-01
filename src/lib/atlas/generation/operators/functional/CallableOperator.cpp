// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "CallableOperator.hpp"

#include "atlas/generation/core/ClassInfo.hpp"
#include "atlas/generation/core/TemplateRegistry.hpp"

namespace wjh::atlas::generation { inline namespace v1 {

std::string_view
CallableOperator::
get_template_impl() const noexcept
{
    static constexpr std::string_view tmpl = R"(
    /**
     * A call operator that takes an invocable, which is then invoked with the
     * wrapped object.
     */
#if defined(__cpp_lib_invoke) && __cpp_lib_invoke >= 201411L
    template <typename InvocableT>
    {{{const_expr}}}auto operator () (InvocableT && inv) const
    noexcept(noexcept(std::invoke(std::forward<InvocableT>(inv), value)))
    -> decltype(std::invoke(std::forward<InvocableT>(inv), value))
    {
        return std::invoke(std::forward<InvocableT>(inv), value);
    }
    template <typename InvocableT>
    {{{const_expr}}}auto operator () (InvocableT && inv)
    noexcept(noexcept(std::invoke(std::forward<InvocableT>(inv), value)))
    -> decltype(std::invoke(std::forward<InvocableT>(inv), value))
    {
        return std::invoke(std::forward<InvocableT>(inv), value);
    }
#else
    template <typename InvocableT>
    {{{const_expr}}}auto operator () (InvocableT && inv) const
    noexcept(noexcept(std::forward<InvocableT>(inv)(value)))
    -> decltype(std::forward<InvocableT>(inv)(value))
    {
        return std::forward<InvocableT>(inv)(value);
    }
    template <typename InvocableT>
    {{{const_expr}}}auto operator () (InvocableT && inv)
    noexcept(noexcept(std::forward<InvocableT>(inv)(value)))
    -> decltype(std::forward<InvocableT>(inv)(value))
    {
        return std::forward<InvocableT>(inv)(value);
    }
#endif
)";
    return tmpl;
}

bool
CallableOperator::
should_apply_impl(ClassInfo const & info) const
{
    return info.callable;
}

boost::json::object
CallableOperator::
prepare_variables_impl(ClassInfo const & info) const
{
    boost::json::object variables;
    variables["const_expr"] = info.const_expr;

    return variables;
}

// Self-registration with the template registry
namespace {
TemplateRegistrar<CallableOperator> callable_operator_registrar;
}

}} // namespace wjh::atlas::generation::v1
