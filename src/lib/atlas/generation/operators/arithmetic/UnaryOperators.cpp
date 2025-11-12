// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "UnaryOperators.hpp"

#include <boost/json/object.hpp>
#include <boost/mustache.hpp>

#include "atlas/generation/core/ClassInfo.hpp"
#include "atlas/generation/core/TemplateRegistry.hpp"

#include <sstream>

namespace wjh::atlas::generation {

// ============================================================================
// UnaryOperatorsTemplate Implementation
// ============================================================================

std::string_view
UnaryOperatorsTemplate::
get_template_impl() const noexcept
{
    static constexpr std::string_view tmpl = R"(
    /**
     * Apply the unary {{{op}}} operator to the wrapped object.
     */
    friend {{{const_expr}}}{{{class_name}}} operator {{{op}}} ({{{class_name}}} const & t)
    noexcept(
        noexcept({{{op}}} std::declval<{{{underlying_type}}} const&>()) &&
        std::is_nothrow_assignable<
            {{{underlying_type}}}&,
            decltype({{{op}}} std::declval<{{{underlying_type}}} const&>())>::value)
    {
        auto result = t;
        result.value = {{{op}}} t.value;
        return result;
    }
)";
    return tmpl;
}

bool
UnaryOperatorsTemplate::
should_apply_impl(ClassInfo const & info) const
{
    return not info.unary_operators.empty();
}

boost::json::object
UnaryOperatorsTemplate::
prepare_variables_for_operator(
    ClassInfo const & info,
    std::string_view op_symbol) const
{
    boost::json::object variables;
    variables["class_name"] = info.class_name;
    variables["underlying_type"] = info.underlying_type;
    variables["const_expr"] = info.const_expr;
    variables["op"] = op_symbol;
    return variables;
}

std::string
UnaryOperatorsTemplate::
render_impl(ClassInfo const & info) const
{
    // Validate that this template should be applied
    validate(info);

    if (not should_apply(info)) {
        return "";
    }

    // Get the template string once
    std::string_view tmpl_str = get_template();
    if (tmpl_str.empty()) {
        return "";
    }

    // Iterate over all unary operators and render each one
    std::ostringstream accumulated;
    for (auto const & op : info.unary_operators) {
        // Prepare variables for this specific operator
        boost::json::object variables = prepare_variables_for_operator(
            info,
            op.op);

        // Render this operator
        std::ostringstream oss;
        boost::mustache::render(
            tmpl_str,
            oss,
            variables,
            boost::json::object{}); // No partials needed

        accumulated << oss.str();
    }

    return accumulated.str();
}

// ============================================================================
// Self-Registration
// ============================================================================

namespace {

TemplateRegistrar<UnaryOperatorsTemplate> register_unary_operators;

} // anonymous namespace

} // namespace wjh::atlas::generation
