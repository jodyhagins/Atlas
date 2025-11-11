// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "NullaryOperator.hpp"

#include "atlas/generation/core/ClassInfo.hpp"
#include "atlas/generation/core/TemplateRegistry.hpp"

namespace wjh::atlas::generation { inline namespace v1 {

std::string_view
NullaryOperator::
get_template_impl() const noexcept
{
    static constexpr std::string_view tmpl = R"(
    /**
     * A nullary call operator that returns access to the wrapped type.
     */
    {{{const_expr}}}{{{underlying_type}}} const & operator () () const
    noexcept
    {
        return value;
    }
    {{{const_expr}}}{{{underlying_type}}} & operator () ()
    noexcept
    {
        return value;
    }
)";
    return tmpl;
}

bool
NullaryOperator::
should_apply_impl(ClassInfo const & info) const
{
    return info.nullary;
}

// Self-registration with the template registry
namespace {
TemplateRegistrar<NullaryOperator> nullary_operator_registrar;
}

}} // namespace wjh::atlas::generation::v1
