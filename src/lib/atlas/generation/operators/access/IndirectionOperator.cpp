// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "IndirectionOperator.hpp"

#include "atlas/generation/core/ClassInfo.hpp"
#include "atlas/generation/core/TemplateRegistry.hpp"

namespace wjh::atlas::generation { inline namespace v1 {

std::string_view
IndirectionOperator::
get_template_impl() const noexcept
{
    static constexpr std::string_view tmpl = R"(
    /**
     * Dereference operator - forwards to wrapped type's operator* if available,
     * otherwise returns reference to wrapped value.
     *
     * Pointer types: dereferences the pointer (returns *ptr)
     * Pointer-like types (smart pointers, iterators, optional): returns *value
     * Other types: returns reference to value (fallback)
     */
    template <typename T = atlas::atlas_detail::const_>
    {{{const_expr}}}auto operator * () const
    -> decltype(atlas::atlas_detail::star_impl<T>(
        value,
        atlas::atlas_detail::PriorityTag<1>{}))
    {
        return atlas::atlas_detail::star_impl<T>(
            value,
            atlas::atlas_detail::PriorityTag<1>{});
    }

    template <typename T = atlas::atlas_detail::mutable_>
    {{{const_expr}}}auto operator * ()
    -> decltype(atlas::atlas_detail::star_impl<T>(
        value,
        atlas::atlas_detail::PriorityTag<10>{}))
    {
        return atlas::atlas_detail::star_impl<T>(
            value,
            atlas::atlas_detail::PriorityTag<10>{});
    }
)";
    return tmpl;
}

bool
IndirectionOperator::
should_apply_impl(ClassInfo const & info) const
{
    return info.indirection_operator;
}

boost::json::object
IndirectionOperator::
prepare_variables_impl(ClassInfo const & info) const
{
    boost::json::object variables;
    variables["const_expr"] = info.const_expr;
    return variables;
}

// Self-registration with the template registry
namespace {
TemplateRegistrar<IndirectionOperator> indirection_operator_registrar;
}

}} // namespace wjh::atlas::generation::v1
