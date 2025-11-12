// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "AdditionOperator.hpp"
#include "BinaryOperatorHelpers.hpp"

#include "atlas/generation/core/TemplateRegistry.hpp"

namespace wjh::atlas::generation { inline namespace v1 {

using namespace arithmetic_helpers;

// ============================================================================
// AdditionOperatorBase Implementation
// ============================================================================

bool
AdditionOperatorBase::
should_apply_impl(ClassInfo const & info) const
{
    return has_binary_operator(info, "+");
}

boost::json::object
AdditionOperatorBase::
prepare_variables_impl(ClassInfo const & info) const
{
    return prepare_binary_operator_variables(info, "+");
}

// ============================================================================
// DefaultAdditionOperator Implementation
// ============================================================================

std::string_view
DefaultAdditionOperator::
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
DefaultAdditionOperator::
should_apply_impl(ClassInfo const & info) const
{
    return has_binary_operator_with_mode(info, "+", ArithmeticMode::Default);
}

// ============================================================================
// CheckedAdditionOperator Implementation
// ============================================================================

std::string_view
CheckedAdditionOperator::
get_template_impl() const noexcept
{
    static constexpr std::string_view tmpl = R"__(
    /**
     * @brief Checked addition - throws on overflow
     * @throws atlas::CheckedOverflowError if result would overflow
     * @throws atlas::CheckedUnderflowError if result would underflow (signed only)
     */
    friend {{{class_name}}} operator + (
        {{{class_name}}} lhs,
        {{{class_name}}} const & rhs)
    {
        lhs.value = atlas::atlas_detail::checked_add(
            lhs.value,
            rhs.value,
            "{{{full_qualified_name}}}: addition overflow",
            "{{{full_qualified_name}}}: addition underflow");
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
CheckedAdditionOperator::
should_apply_impl(ClassInfo const & info) const
{
    return has_binary_operator_with_mode(info, "+", ArithmeticMode::Checked);
}

// ============================================================================
// SaturatingAdditionOperator Implementation
// ============================================================================

std::string_view
SaturatingAdditionOperator::
get_template_impl() const noexcept
{
    static constexpr std::string_view tmpl = R"__(
    /**
     * @brief Saturating addition - clamps to type limits
     * @note noexcept - overflow/underflow clamps to limits instead of throwing
     */
    friend {{{class_name}}} operator + (
        {{{class_name}}} lhs,
        {{{class_name}}} const & rhs)
    {{^has_constraint}}
    noexcept
    {{/has_constraint}}
    {
        lhs.value = atlas::atlas_detail::saturating_add(lhs.value, rhs.value);
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
SaturatingAdditionOperator::
should_apply_impl(ClassInfo const & info) const
{
    return has_binary_operator_with_mode(info, "+", ArithmeticMode::Saturating);
}

// ============================================================================
// WrappingAdditionOperator Implementation
// ============================================================================

std::string_view
WrappingAdditionOperator::
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
WrappingAdditionOperator::
should_apply_impl(ClassInfo const & info) const
{
    return has_binary_operator_with_mode(info, "+", ArithmeticMode::Wrapping);
}

// ============================================================================
// Self-Registration
// ============================================================================

namespace {

/**
 * Self-registering instances of addition operator templates
 *
 * These static instances register each addition mode with the TemplateRegistry
 * during static initialization (before main() starts).
 */
TemplateRegistrar<DefaultAdditionOperator> register_default_addition;
TemplateRegistrar<CheckedAdditionOperator> register_checked_addition;
TemplateRegistrar<SaturatingAdditionOperator> register_saturating_addition;
TemplateRegistrar<WrappingAdditionOperator> register_wrapping_addition;

} // anonymous namespace

}} // namespace wjh::atlas::generation::v1
