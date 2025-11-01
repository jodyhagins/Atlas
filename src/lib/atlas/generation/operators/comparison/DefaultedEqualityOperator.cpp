// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "DefaultedEqualityOperator.hpp"

#include "atlas/generation/core/ClassInfo.hpp"
#include "atlas/generation/core/TemplateRegistry.hpp"

namespace wjh::atlas::generation { inline namespace v1 {

std::string_view
DefaultedEqualityOperator::
get_template_impl() const noexcept
{
    static constexpr std::string_view tmpl = R"(
#if defined(__cpp_impl_three_way_comparison) && \
    __cpp_impl_three_way_comparison >= 201907L
    /**
     * The default equality comparison operator.
     * Provided with spaceship operator for optimal performance.
     */
    friend {{{const_expr}}}bool operator == (
        {{{class_name}}} const &,
        {{{class_name}}} const &) = default;
#else
    /**
     * Equality comparison operators (C++17 fallback).
     * In C++20+, these are synthesized from operator<=>.
     */
    friend {{{const_expr}}}bool operator == (
        {{{class_name}}} const & lhs,
        {{{class_name}}} const & rhs)
    noexcept(noexcept(std::declval<{{{underlying_type}}} const &>() ==
        std::declval<{{{underlying_type}}} const &>()))
    {
        return lhs.value == rhs.value;
    }

    friend {{{const_expr}}}bool operator != (
        {{{class_name}}} const & lhs,
        {{{class_name}}} const & rhs)
    noexcept(noexcept(std::declval<{{{underlying_type}}} const &>() !=
        std::declval<{{{underlying_type}}} const &>()))
    {
        return lhs.value != rhs.value;
    }
#endif
)";
    return tmpl;
}

bool
DefaultedEqualityOperator::
should_apply_impl(ClassInfo const & info) const
{
    // Parse to get ClassInfo and check if defaulted equality operator is
    // enabled
    return info.defaulted_equality_operator;
}

boost::json::object
DefaultedEqualityOperator::
prepare_variables_impl(ClassInfo const & info) const
{
    // Parse to get ClassInfo
    boost::json::object variables;
    variables["class_name"] = info.class_name;
    variables["underlying_type"] = info.underlying_type;
    variables["const_expr"] = info.const_expr;
    return variables;
}

// ============================================================================
// Self-Registration
// ============================================================================

namespace {

/**
 * Self-registering instance of DefaultedEqualityOperator template
 *
 * This static instance registers the DefaultedEqualityOperator with the
 * TemplateRegistry during static initialization (before main() starts).
 */
TemplateRegistrar<DefaultedEqualityOperator>
    register_defaulted_equality_operator;

} // anonymous namespace

}} // namespace wjh::atlas::generation::v1
