// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "LogicalNotOperator.hpp"

#include "atlas/generation/core/ClassInfo.hpp"
#include "atlas/generation/core/TemplateRegistry.hpp"

namespace wjh::atlas::generation { inline namespace v1 {

std::string_view
LogicalNotOperator::
get_template_impl() const noexcept
{
    static constexpr std::string_view tmpl = R"(
    /**
     * Apply the unary logical not operator to the wrapped object.
     */
    friend {{{const_expr}}}bool operator not ({{{class_name}}} const & t)
    noexcept(noexcept(not std::declval<{{{underlying_type}}} const&>()))
    {
        return not t.value;
    }
)";
    return tmpl;
}

bool
LogicalNotOperator::
should_apply_impl(ClassInfo const & info) const
{
    return info.logical_not_operator;
}

boost::json::object
LogicalNotOperator::
prepare_variables_impl(ClassInfo const & info) const
{
    boost::json::object variables;
    variables["const_expr"] = info.const_expr;
    variables["class_name"] = info.class_name;
    variables["underlying_type"] = info.underlying_type;

    return variables;
}

// Self-registration with the template registry
namespace {
TemplateRegistrar<LogicalNotOperator> logical_not_operator_registrar;
}

}} // namespace wjh::atlas::generation::v1
