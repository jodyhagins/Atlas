// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "SubscriptOperator.hpp"

#include "atlas/generation/core/ClassInfo.hpp"
#include "atlas/generation/core/TemplateRegistry.hpp"

namespace wjh::atlas::generation { inline namespace v1 {

std::string_view
SubscriptOperator::
get_template_impl() const noexcept
{
    static constexpr std::string_view tmpl = R"(
    /**
     * Subscript operator that forwards to the wrapped object.
     */
#if __cpp_multidimensional_subscript >= 202110L
    template <typename ArgT, typename... ArgTs>
    {{{const_expr}}}decltype(auto) operator [] (ArgT && arg, ArgTs && ... args)
    noexcept(noexcept(value[std::forward<ArgT>(arg), std::forward<ArgTs>(args)...]))
    {
        return value[std::forward<ArgT>(arg), std::forward<ArgTs>(args)...];
    }
    template <typename ArgT, typename... ArgTs>
    {{{const_expr}}}decltype(auto) operator [] (ArgT && arg, ArgTs && ... args) const
    noexcept(noexcept(value[std::forward<ArgT>(arg), std::forward<ArgTs>(args)...]))
    {
        return value[std::forward<ArgT>(arg), std::forward<ArgTs>(args)...];
    }
#else
    template <typename ArgT>
    {{{const_expr}}}auto operator [] (ArgT && arg)
    noexcept(noexcept(value[std::forward<ArgT>(arg)]))
    -> decltype(value[std::forward<ArgT>(arg)])
    {
        return value[std::forward<ArgT>(arg)];
    }
    template <typename ArgT>
    {{{const_expr}}}auto operator [] (ArgT && arg) const
    noexcept(noexcept(value[std::forward<ArgT>(arg)]))
    -> decltype(value[std::forward<ArgT>(arg)])
    {
        return value[std::forward<ArgT>(arg)];
    }
#endif
)";
    return tmpl;
}

bool
SubscriptOperator::
should_apply_impl(ClassInfo const & info) const
{
    return info.subscript_operator;
}

boost::json::object
SubscriptOperator::
prepare_variables_impl(ClassInfo const & info) const
{
    boost::json::object variables;
    variables["const_expr"] = info.const_expr;

    return variables;
}

// Self-registration with the template registry
namespace {
TemplateRegistrar<SubscriptOperator> subscript_operator_registrar;
}

}} // namespace wjh::atlas::generation::v1
