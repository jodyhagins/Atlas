// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "BinaryOperatorHelpers.hpp"
#include "DivisionOperator.hpp"

#include "atlas/generation/core/TemplateRegistry.hpp"

namespace wjh::atlas::generation {

using namespace arithmetic_helpers;

// ============================================================================
// DivisionOperatorBase Implementation
// ============================================================================

bool
DivisionOperatorBase::
should_apply_impl(ClassInfo const & info) const
{
    return has_binary_operator(info, "/");
}

boost::json::object
DivisionOperatorBase::
prepare_variables_impl(ClassInfo const & info) const
{
    return prepare_binary_operator_variables(info, "/");
}

// ============================================================================
// DefaultDivisionOperator Implementation
// ============================================================================

std::string_view
DefaultDivisionOperator::
get_template_impl() const noexcept
{
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

bool
DefaultDivisionOperator::
should_apply_impl(ClassInfo const & info) const
{
    // Division uses default operator for both Default and Wrapping modes
    // (wrapping doesn't apply to division the same way as +,-,*)
    return has_binary_operator_with_mode(info, "/", ArithmeticMode::Default) ||
        has_binary_operator_with_mode(info, "/", ArithmeticMode::Wrapping);
}

// ============================================================================
// CheckedDivisionOperator Implementation
// ============================================================================

std::string_view
CheckedDivisionOperator::
get_template_impl() const noexcept
{
    static constexpr std::string_view tmpl = R"__(
    /**
     * @brief Checked division - throws on division by zero and overflow
     * @throws atlas::CheckedDivisionByZeroError if divisor is zero
     * @throws atlas::CheckedOverflowError if result would overflow (INT_MIN / -1)
     */
    friend {{{class_name}}} operator / (
        {{{class_name}}} lhs,
        {{{class_name}}} const & rhs)
    {
        lhs.value = atlas::atlas_detail::checked_div(
            lhs.value,
            rhs.value,
            "{{{full_qualified_name}}}: division by zero",
            "{{{full_qualified_name}}}: division overflow (INT_MIN / -1)");
        {{#has_constraint}}
        if (not atlas_constraint::check(lhs.value)) {
            throw atlas::ConstraintError(
                "{{{class_name}}}: arithmetic result violates constraint"
                " ({{{constraint_message}}})");
        }
        {{/has_constraint}}
        return lhs;
    }
)__";
    return tmpl;
}

bool
CheckedDivisionOperator::
should_apply_impl(ClassInfo const & info) const
{
    return has_binary_operator_with_mode(info, "/", ArithmeticMode::Checked);
}

// ============================================================================
// SaturatingDivisionOperator Implementation
// ============================================================================

std::string_view
SaturatingDivisionOperator::
get_template_impl() const noexcept
{
    static constexpr std::string_view tmpl = R"__(
    /**
     * @brief Saturating division - clamps to type limits
     * @note noexcept - overflow/underflow clamps to limits instead of throwing
     */
    friend {{{class_name}}} operator / (
        {{{class_name}}} lhs,
        {{{class_name}}} const & rhs)
    {{^has_constraint}}
    noexcept
    {{/has_constraint}}
    {
        lhs.value = atlas::atlas_detail::saturating_div(lhs.value, rhs.value);
        {{#has_constraint}}
        if (not atlas_constraint::check(lhs.value)) {
            throw atlas::ConstraintError(
                "{{{class_name}}}: arithmetic result violates constraint"
                " ({{{constraint_message}}})");
        }
        {{/has_constraint}}
        return lhs;
    }
)__";
    return tmpl;
}

bool
SaturatingDivisionOperator::
should_apply_impl(ClassInfo const & info) const
{
    return has_binary_operator_with_mode(info, "/", ArithmeticMode::Saturating);
}

// ============================================================================
// Self-Registration
// ============================================================================

namespace {

TemplateRegistrar<DefaultDivisionOperator> register_default_division;
TemplateRegistrar<CheckedDivisionOperator> register_checked_division;
TemplateRegistrar<SaturatingDivisionOperator> register_saturating_division;

} // anonymous namespace

} // namespace wjh::atlas::generation
