// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "BitwiseOperators.hpp"

#include <boost/json/object.hpp>

#include "atlas/generation/core/ClassInfo.hpp"
#include "atlas/generation/core/TemplateRegistry.hpp"

#include <algorithm>

namespace wjh::atlas::generation { inline namespace v1 {

// ============================================================================
// Helper Functions
// ============================================================================

namespace {

bool
has_binary_operator(ClassInfo const & info, std::string_view op_symbol)
{
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

} // anonymous namespace

// ============================================================================
// BitwiseOperatorBase Implementation
// ============================================================================

std::string_view
BitwiseOperatorBase::
get_template_impl() const noexcept
{
    // All bitwise operators use the same template as default arithmetic
    // operators
    static constexpr std::string_view tmpl = R"__(
    /**
     * Apply {{{op}}} assignment to the wrapped objects.
     */
    friend {{{const_expr}}}{{{class_name}}} & operator {{{op}}}= (
        {{{class_name}}} & lhs,
        {{{class_name}}} const & rhs)
{{^has_constraint}}
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunevaluated-expression"
#endif
    noexcept(noexcept(std::declval<{{{underlying_type}}} &>() {{{op}}}= std::declval<{{{underlying_type}}} const &>()))
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
{{/has_constraint}}
    {
        lhs.{{{value}}} {{{op}}}= rhs.{{{value}}};
        {{#has_constraint}}
        if (not atlas_constraint::check(lhs.{{{value}}})) {
            throw atlas::ConstraintError(
                "{{{class_name}}}: arithmetic result violates constraint"
                " ({{{constraint_message}}})");
        }
        {{/has_constraint}}
        return lhs;
    }
    /**
     * Apply the binary operator {{{op}}} to the wrapped object.
     */
    friend {{{const_expr}}}{{{class_name}}} operator {{{op}}} (
        {{{class_name}}} lhs,
        {{{class_name}}} const & rhs)
    noexcept(noexcept(lhs {{{op}}}= rhs))
    {
        lhs {{{op}}}= rhs;
        return lhs;
    }
)__";
    return tmpl;
}

boost::json::object
BitwiseOperatorBase::
prepare_variables_for_operator(
    ClassInfo const & info,
    std::string_view op_symbol) const
{
    return prepare_binary_operator_variables(info, op_symbol);
}

// ============================================================================
// Bitwise AND (&)
// ============================================================================

bool
BitwiseAndOperatorBase::
should_apply_impl(ClassInfo const & info) const
{
    return has_binary_operator(info, "&");
}

boost::json::object
BitwiseAndOperatorBase::
prepare_variables_impl(ClassInfo const & info) const
{
    return BitwiseOperatorBase::prepare_variables_for_operator(info, "&");
}

bool
DefaultBitwiseAndOperator::
should_apply_impl(ClassInfo const & info) const
{
    return has_binary_operator_with_mode(info, "&", ArithmeticMode::Default);
}

// ============================================================================
// Bitwise OR (|)
// ============================================================================

bool
BitwiseOrOperatorBase::
should_apply_impl(ClassInfo const & info) const
{
    return has_binary_operator(info, "|");
}

boost::json::object
BitwiseOrOperatorBase::
prepare_variables_impl(ClassInfo const & info) const
{
    return BitwiseOperatorBase::prepare_variables_for_operator(info, "|");
}

bool
DefaultBitwiseOrOperator::
should_apply_impl(ClassInfo const & info) const
{
    return has_binary_operator_with_mode(info, "|", ArithmeticMode::Default);
}

// ============================================================================
// Bitwise XOR (^)
// ============================================================================

bool
BitwiseXorOperatorBase::
should_apply_impl(ClassInfo const & info) const
{
    return has_binary_operator(info, "^");
}

boost::json::object
BitwiseXorOperatorBase::
prepare_variables_impl(ClassInfo const & info) const
{
    return BitwiseOperatorBase::prepare_variables_for_operator(info, "^");
}

bool
DefaultBitwiseXorOperator::
should_apply_impl(ClassInfo const & info) const
{
    return has_binary_operator_with_mode(info, "^", ArithmeticMode::Default);
}

// ============================================================================
// Left Shift (<<)
// ============================================================================

bool
LeftShiftOperatorBase::
should_apply_impl(ClassInfo const & info) const
{
    return has_binary_operator(info, "<<");
}

boost::json::object
LeftShiftOperatorBase::
prepare_variables_impl(ClassInfo const & info) const
{
    return BitwiseOperatorBase::prepare_variables_for_operator(info, "<<");
}

bool
DefaultLeftShiftOperator::
should_apply_impl(ClassInfo const & info) const
{
    return has_binary_operator_with_mode(info, "<<", ArithmeticMode::Default);
}

// ============================================================================
// Right Shift (>>)
// ============================================================================

bool
RightShiftOperatorBase::
should_apply_impl(ClassInfo const & info) const
{
    return has_binary_operator(info, ">>");
}

boost::json::object
RightShiftOperatorBase::
prepare_variables_impl(ClassInfo const & info) const
{
    return BitwiseOperatorBase::prepare_variables_for_operator(info, ">>");
}

bool
DefaultRightShiftOperator::
should_apply_impl(ClassInfo const & info) const
{
    return has_binary_operator_with_mode(info, ">>", ArithmeticMode::Default);
}

// ============================================================================
// Self-Registration
// ============================================================================

namespace {

TemplateRegistrar<DefaultBitwiseAndOperator> register_bitwise_and;
TemplateRegistrar<DefaultBitwiseOrOperator> register_bitwise_or;
TemplateRegistrar<DefaultBitwiseXorOperator> register_bitwise_xor;
TemplateRegistrar<DefaultLeftShiftOperator> register_left_shift;
TemplateRegistrar<DefaultRightShiftOperator> register_right_shift;

} // anonymous namespace

}} // namespace wjh::atlas::generation::v1
