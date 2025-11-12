// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "SpaceshipOperator.hpp"

#include "atlas/generation/core/ClassInfo.hpp"
#include "atlas/generation/core/TemplateRegistry.hpp"

namespace wjh::atlas::generation {

std::string_view
SpaceshipOperator::
get_template_impl() const noexcept
{
    static constexpr std::string_view tmpl = R"(
#if defined(__cpp_impl_three_way_comparison) && \
    __cpp_impl_three_way_comparison >= 201907L
    /**
     * The default three-way comparison (spaceship) operator.
     */
    friend {{{const_expr}}}auto operator <=> (
        {{{class_name}}} const &,
        {{{class_name}}} const &) = default;
#else
    /**
     * Comparison operators (C++17 fallback for spaceship operator).
     * In C++20+, these are synthesized from operator<=>.
     */
    friend {{{const_expr}}}bool operator < (
        {{{class_name}}} const & lhs,
        {{{class_name}}} const & rhs)
    noexcept(noexcept(std::declval<{{{underlying_type}}} const &>() <
        std::declval<{{{underlying_type}}} const &>()))
    {
        return lhs.{{{value}}} < rhs.{{{value}}};
    }

    friend {{{const_expr}}}bool operator <= (
        {{{class_name}}} const & lhs,
        {{{class_name}}} const & rhs)
    noexcept(noexcept(std::declval<{{{underlying_type}}} const &>() <=
        std::declval<{{{underlying_type}}} const &>()))
    {
        return lhs.{{{value}}} <= rhs.{{{value}}};
    }

    friend {{{const_expr}}}bool operator > (
        {{{class_name}}} const & lhs,
        {{{class_name}}} const & rhs)
    noexcept(noexcept(std::declval<{{{underlying_type}}} const &>() >
        std::declval<{{{underlying_type}}} const &>()))
    {
        return lhs.{{{value}}} > rhs.{{{value}}};
    }

    friend {{{const_expr}}}bool operator >= (
        {{{class_name}}} const & lhs,
        {{{class_name}}} const & rhs)
    noexcept(noexcept(std::declval<{{{underlying_type}}} const &>() >=
        std::declval<{{{underlying_type}}} const &>()))
    {
        return lhs.{{{value}}} >= rhs.{{{value}}};
    }
#endif
)";
    return tmpl;
}

bool
SpaceshipOperator::
should_apply_impl(ClassInfo const & info) const
{
    // Parse to get ClassInfo and check if spaceship operator is enabled
    return info.spaceship_operator;
}

std::set<std::string>
SpaceshipOperator::
required_includes_impl() const
{
    // The <compare> header is only needed for C++20 three-way comparison
    // support. The C++17 fallback doesn't require this header (it uses basic
    // comparison operators). We include it unconditionally because the code
    // generation happens at C++20 compile time, and the generated code uses
    // preprocessor conditionals to select the right path. If this causes issues
    // on C++17-only systems, the feature test macro in the generated code will
    // select the fallback implementation that doesn't need <compare>.
    return {"<compare>"};
}

// ============================================================================
// Self-Registration
// ============================================================================

namespace {

/**
 * Self-registering instance of SpaceshipOperator template
 *
 * This static instance registers the SpaceshipOperator with the
 * TemplateRegistry during static initialization (before main() starts).
 */
TemplateRegistrar<SpaceshipOperator> register_spaceship_operator;

} // anonymous namespace

} // namespace wjh::atlas::generation
