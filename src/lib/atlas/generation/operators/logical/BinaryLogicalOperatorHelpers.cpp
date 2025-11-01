// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "BinaryLogicalOperatorHelpers.hpp"

#include "atlas/generation/core/ClassInfo.hpp"

#include <algorithm>

namespace wjh::atlas::generation { inline namespace v1 {
namespace logical_helpers {

bool
has_binary_logical_operator(ClassInfo const & info, std::string_view op_symbol)
{
    return std::any_of(
        info.logical_operators.begin(),
        info.logical_operators.end(),
        [op_symbol](auto const & op) { return op.op == op_symbol; });
}

boost::json::object
prepare_binary_logical_operator_variables(
    ClassInfo const & info,
    std::string_view op_symbol)
{
    boost::json::object variables;
    variables["const_expr"] = info.const_expr;
    variables["class_name"] = info.class_name;
    variables["underlying_type"] = info.underlying_type;
    variables["op"] = op_symbol;

    return variables;
}

std::string_view
get_binary_logical_operator_template() noexcept
{
    static constexpr std::string_view tmpl = R"(
    /**
     * Apply the binary logical operator {{{op}}} to the wrapped object.
     *
     * @note  General advice is to NOT overload these operators.
     * One of the reasons is that short-circuit is no longer available.
     * Proceed with caution.
     */
    friend {{{const_expr}}}bool operator {{{op}}} (
        {{{class_name}}} const & lhs,
        {{{class_name}}} const & rhs)
    noexcept(noexcept(std::declval<{{{underlying_type}}} const&>() {{{op}}} std::declval<{{{underlying_type}}} const&>()))
    {
        return lhs.value {{{op}}} rhs.value;
    }
)";
    return tmpl;
}

}}} // namespace wjh::atlas::generation::v1::logical_helpers
