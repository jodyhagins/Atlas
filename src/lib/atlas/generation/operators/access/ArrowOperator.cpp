// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "ArrowOperator.hpp"

#include "atlas/generation/core/ClassInfo.hpp"
#include "atlas/generation/core/TemplateRegistry.hpp"

namespace wjh::atlas::generation { inline namespace v1 {

std::string_view
ArrowOperator::
get_template_impl() const noexcept
{
    static constexpr std::string_view tmpl = R"(
    /**
     * Arrow operator - forwards to wrapped type if it's a pointer or
     * pointer-like, otherwise returns pointer to wrapped value.
     *
     * pointer types: returns the pointer itself (built-in -> handles the rest)
     * pointer-like types (smart pointers): returns value.operator->()
     * other types: returns &value
     */
    template <typename T = atlas::atlas_detail::const_>
    {{{const_expr}}}auto operator -> () const
    -> decltype(atlas::atlas_detail::arrow_impl<T>(
        value,
        atlas::atlas_detail::PriorityTag<1>{}))
    {
        return atlas::atlas_detail::arrow_impl<T>(
            value,
            atlas::atlas_detail::PriorityTag<1>{});
    }

    template <typename T = atlas::atlas_detail::mutable_>
    {{{const_expr}}}auto operator -> ()
    -> decltype(atlas::atlas_detail::arrow_impl<T>(
        value,
        atlas::atlas_detail::PriorityTag<1>{}))
    {
        return atlas::atlas_detail::arrow_impl<T>(
            value,
            atlas::atlas_detail::PriorityTag<1>{});
    }
)";
    return tmpl;
}

bool
ArrowOperator::
should_apply_impl(ClassInfo const & info) const
{
    return info.arrow_operator;
}

boost::json::object
ArrowOperator::
prepare_variables_impl(ClassInfo const & info) const
{
    boost::json::object variables;
    variables["const_expr"] = info.const_expr;
    return variables;
}

// Self-registration with the template registry
namespace {
TemplateRegistrar<ArrowOperator> arrow_operator_registrar;
}

}} // namespace wjh::atlas::generation::v1
