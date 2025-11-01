// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "BinaryOperatorHelpers.hpp"
#include "ModuloOperator.hpp"

#include "atlas/generation/core/TemplateRegistry.hpp"

namespace wjh::atlas::generation { inline namespace v1 {

using namespace arithmetic_helpers;

// ============================================================================
// ModuloOperatorBase Implementation
// ============================================================================

bool
ModuloOperatorBase::
should_apply_impl(ClassInfo const & info) const
{
    return has_binary_operator(info, "%");
}

boost::json::object
ModuloOperatorBase::
prepare_variables_impl(ClassInfo const & info) const
{
    return prepare_binary_operator_variables(info, "%");
}

// ============================================================================
// DefaultModuloOperator Implementation
// ============================================================================

std::string_view
DefaultModuloOperator::
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
        lhs.value {{{op}}}= rhs.value;
        {{#has_constraint}}
        if (not atlas_constraint::check(lhs.value)) {
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
DefaultModuloOperator::
should_apply_impl(ClassInfo const & info) const
{
    // Modulo uses default operator for both Default and Wrapping modes
    // (wrapping doesn't apply to modulo the same way as +,-,*)
    return has_binary_operator_with_mode(info, "%", ArithmeticMode::Default) ||
        has_binary_operator_with_mode(info, "%", ArithmeticMode::Wrapping);
}

// ============================================================================
// CheckedModuloOperator Implementation
// ============================================================================

std::string_view
CheckedModuloOperator::
get_template_impl() const noexcept
{
    static constexpr std::string_view tmpl = R"__(
    /**
     * @brief Checked modulo - throws on division by zero
     * @throws atlas::CheckedDivisionByZeroError if divisor is zero
     * @note Modulo is only defined for integral types
     */
    friend {{{class_name}}} operator % (
        {{{class_name}}} lhs,
        {{{class_name}}} const & rhs)
    {
        lhs.value = atlas::atlas_detail::checked_mod(
            lhs.value,
            rhs.value,
            "{{{full_qualified_name}}}: modulo by zero");
        {{#has_constraint}}
        if (not atlas_constraint::check(lhs.value)) {
            throw atlas::ConstraintError(
                "{{{class_name}}}: arithmetic result violates constraint"
                "({{{constraint_message}}})");
        }
        {{/has_constraint}}
        return lhs;
    }
)__";
    return tmpl;
}

bool
CheckedModuloOperator::
should_apply_impl(ClassInfo const & info) const
{
    return has_binary_operator_with_mode(info, "%", ArithmeticMode::Checked);
}

// ============================================================================
// SaturatingModuloOperator Implementation
// ============================================================================

std::string_view
SaturatingModuloOperator::
get_template_impl() const noexcept
{
    static constexpr std::string_view tmpl = R"__(
    /**
     * @brief Saturating modulo - clamps to type limits
     * @note noexcept - overflow clamps to limits instead of throwing
     * @note Modulo can only produce values in range [0, rhs), so saturation
     *       is essentially just returning 0 on divide-by-zero
     */
    friend {{{class_name}}} operator % (
        {{{class_name}}} lhs,
        {{{class_name}}} const & rhs)
    {{^has_constraint}}
    noexcept
    {{/has_constraint}}
    {
        lhs.value = atlas::atlas_detail::saturating_rem(lhs.value, rhs.value);
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
SaturatingModuloOperator::
should_apply_impl(ClassInfo const & info) const
{
    return has_binary_operator_with_mode(info, "%", ArithmeticMode::Saturating);
}

// ============================================================================
// Self-Registration
// ============================================================================

namespace {

TemplateRegistrar<DefaultModuloOperator> register_default_modulo;
TemplateRegistrar<CheckedModuloOperator> register_checked_modulo;
TemplateRegistrar<SaturatingModuloOperator> register_saturating_modulo;

} // anonymous namespace

}} // namespace wjh::atlas::generation::v1
