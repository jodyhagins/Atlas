// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "BinaryOperatorHelpers.hpp"
#include "SubtractionOperator.hpp"

#include "atlas/generation/core/TemplateRegistry.hpp"

namespace wjh::atlas::generation { inline namespace v1 {

using namespace arithmetic_helpers;

// ============================================================================
// SubtractionOperatorBase Implementation
// ============================================================================

bool
SubtractionOperatorBase::
should_apply_impl(ClassInfo const & info) const
{
    return has_binary_operator(info, "-");
}

boost::json::object
SubtractionOperatorBase::
prepare_variables_impl(ClassInfo const & info) const
{
    return prepare_binary_operator_variables(info, "-");
}

// ============================================================================
// DefaultSubtractionOperator Implementation
// ============================================================================

std::string_view
DefaultSubtractionOperator::
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
DefaultSubtractionOperator::
should_apply_impl(ClassInfo const & info) const
{
    return has_binary_operator_with_mode(info, "-", ArithmeticMode::Default);
}

// ============================================================================
// CheckedSubtractionOperator Implementation
// ============================================================================

std::string_view
CheckedSubtractionOperator::
get_template_impl() const noexcept
{
    static constexpr std::string_view tmpl = R"__(
    /**
     * @brief Checked subtraction - throws on overflow/underflow
     * @throws atlas::CheckedOverflowError if result would overflow
     * @throws atlas::CheckedUnderflowError if result would underflow
     */
    friend {{{class_name}}} operator - (
        {{{class_name}}} lhs,
        {{{class_name}}} const & rhs)
    {
        lhs.value = atlas::atlas_detail::checked_sub(
            lhs.value,
            rhs.value,
            "{{{full_qualified_name}}}: subtraction overflow",
            "{{{full_qualified_name}}}: subtraction underflow");
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
CheckedSubtractionOperator::
should_apply_impl(ClassInfo const & info) const
{
    return has_binary_operator_with_mode(info, "-", ArithmeticMode::Checked);
}

// ============================================================================
// SaturatingSubtractionOperator Implementation
// ============================================================================

std::string_view
SaturatingSubtractionOperator::
get_template_impl() const noexcept
{
    static constexpr std::string_view tmpl = R"__(
    /**
     * @brief Saturating subtraction - clamps to type limits
     * @note noexcept - overflow/underflow clamps to limits instead of throwing
     */
    friend {{{class_name}}} operator - (
        {{{class_name}}} lhs,
        {{{class_name}}} const & rhs)
    {{^has_constraint}}
    noexcept
    {{/has_constraint}}
    {
        lhs.value = atlas::atlas_detail::saturating_sub(lhs.value, rhs.value);
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
SaturatingSubtractionOperator::
should_apply_impl(ClassInfo const & info) const
{
    return has_binary_operator_with_mode(info, "-", ArithmeticMode::Saturating);
}

// ============================================================================
// WrappingSubtractionOperator Implementation
// ============================================================================

std::string_view
WrappingSubtractionOperator::
get_template_impl() const noexcept
{
    static constexpr std::string_view tmpl = R"__(
    /**
     * @brief Wrapping arithmetic - explicit, well-defined overflow
     * @note Marked noexcept - overflow is intentional and well-defined
     * @note Uses unsigned arithmetic to avoid UB for signed integer overflow
     * @note Only available for integral types
     */
    friend {{{class_name}}} operator {{{op}}} (
        {{{class_name}}} lhs,
        {{{class_name}}} const & rhs)
    {{^has_constraint}}
    noexcept
    {{/has_constraint}}
    {
        static_assert(std::is_integral<{{{underlying_type}}}>::value,
                      "Wrapping arithmetic is only supported for integral types");
        using unsigned_type = typename std::make_unsigned<{{{underlying_type}}}>::type;
        lhs.value = static_cast<{{{underlying_type}}}>(
            static_cast<unsigned_type>(lhs.value) {{{op}}}
            static_cast<unsigned_type>(rhs.value)
        );
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
WrappingSubtractionOperator::
should_apply_impl(ClassInfo const & info) const
{
    return has_binary_operator_with_mode(info, "-", ArithmeticMode::Wrapping);
}

// ============================================================================
// Self-Registration
// ============================================================================

namespace {

TemplateRegistrar<DefaultSubtractionOperator> register_default_subtraction;
TemplateRegistrar<CheckedSubtractionOperator> register_checked_subtraction;
TemplateRegistrar<SaturatingSubtractionOperator>
    register_saturating_subtraction;
TemplateRegistrar<WrappingSubtractionOperator> register_wrapping_subtraction;

} // anonymous namespace

}} // namespace wjh::atlas::generation::v1
