// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "BinaryOperatorHelpers.hpp"

#include <algorithm>

namespace wjh::atlas::generation { inline namespace v1 {

namespace arithmetic_helpers {

bool
has_binary_operator(ClassInfo const & info, std::string_view op_symbol)
{
    // Search for the operator symbol in the binary operators list
    return std::any_of(
        info.arithmetic_binary_operators.begin(),
        info.arithmetic_binary_operators.end(),
        [op_symbol](auto const & op) { return op.op == op_symbol; });
}

bool
has_binary_operator_with_mode(
    ClassInfo const & info,
    std::string_view op_symbol,
    ArithmeticMode mode)
{
    return info.arithmetic_mode == mode && has_binary_operator(info, op_symbol);
}

boost::json::object
prepare_binary_operator_variables(
    ClassInfo const & info,
    std::string_view op_symbol)
{
    // Build JSON object with variables needed for binary operator templates
    // These variables are used by Mustache templates to generate operator code
    boost::json::object vars;
    vars["class_name"] = info.class_name;
    vars["underlying_type"] = info.underlying_type;
    vars["full_qualified_name"] = info.full_qualified_name;
    vars["has_constraint"] = info.has_constraint;
    vars["constraint_message"] = info.constraint_message;
    vars["op"] = op_symbol;
    vars["const_expr"] = info.const_expr;

    return vars;
}

}}} // namespace wjh::atlas::generation::v1::arithmetic_helpers
