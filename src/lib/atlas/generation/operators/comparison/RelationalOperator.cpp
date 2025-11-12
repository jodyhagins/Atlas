// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "RelationalOperator.hpp"

#include <boost/mustache.hpp>

#include "atlas/generation/core/ClassInfo.hpp"
#include "atlas/generation/core/TemplateRegistry.hpp"

#include <sstream>

namespace wjh::atlas::generation { inline namespace v1 {

std::string_view
RelationalOperator::
get_template_impl() const noexcept
{
    static constexpr std::string_view tmpl = R"(
    /**
     * Is @p lhs.value {{{op}}} @p rhs.value?
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

bool
RelationalOperator::
should_apply_impl(ClassInfo const & info) const
{
    return not info.relational_operators.empty();
}

boost::json::object
RelationalOperator::
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
RelationalOperator::
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

    // Iterate over all relational operators and render each one
    std::ostringstream accumulated;
    for (auto const & op : info.relational_operators) {
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
            boost::json::object{});

        // Accumulate into result
        accumulated << oss.str();
    }

    return accumulated.str();
}

// ============================================================================
// Self-Registration
// ============================================================================

namespace {

/**
 * Self-registering instance of RelationalOperator template
 *
 * This static instance registers the RelationalOperator with the
 * TemplateRegistry during static initialization (before main() starts).
 */
TemplateRegistrar<RelationalOperator> register_relational_operator;

} // anonymous namespace

}} // namespace wjh::atlas::generation::v1
