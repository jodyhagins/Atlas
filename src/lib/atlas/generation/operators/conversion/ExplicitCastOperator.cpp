// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "ExplicitCastOperator.hpp"

#include <boost/mustache.hpp>

#include "atlas/generation/core/ClassInfo.hpp"
#include "atlas/generation/core/TemplateRegistry.hpp"

#include <sstream>

namespace wjh::atlas::generation { inline namespace v1 {

std::string
ExplicitCastOperator::
id_impl() const
{
    return "operators.conversion.explicit";
}

std::string_view
ExplicitCastOperator::
get_template_impl() const
{
    static constexpr std::string_view tmpl = R"(
    /**
     * Explicit cast to {{{cast_type}}}
     */
    {{{const_expr}}}explicit operator {{{cast_type}}}() const
    noexcept(noexcept(static_cast<{{{cast_type}}}>(
        std::declval<{{{underlying_type}}} const&>())))
    {
        return static_cast<{{{cast_type}}}>(value);
    }
)";
    return tmpl;
}

bool
ExplicitCastOperator::
should_apply_impl(ClassInfo const & info) const
{
    return not info.explicit_cast_operators.empty();
}

boost::json::object
ExplicitCastOperator::
prepare_variables_impl(ClassInfo const & info) const
{
    // This method is still required by the interface, but it's not used
    // since we override render_impl(). For completeness, return base variables
    // without the "cast_type" field.
    boost::json::object variables;
    variables["const_expr"] = info.const_expr;
    variables["underlying_type"] = info.underlying_type;
    return variables;
}

boost::json::object
ExplicitCastOperator::
prepare_variables_for_cast(ClassInfo const & info, CastOperator const & cast)
    const
{
    boost::json::object variables;
    variables["const_expr"] = info.const_expr;
    variables["underlying_type"] = info.underlying_type;
    variables["cast_type"] = cast.cast_type;
    return variables;
}

std::string
ExplicitCastOperator::
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

    // Iterate over all explicit cast operators and render each one
    std::ostringstream accumulated;
    for (auto const & cast : info.explicit_cast_operators) {
        // Prepare variables for this specific cast
        boost::json::object variables = prepare_variables_for_cast(info, cast);

        // Render this cast operator
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
 * Self-registering instance of ExplicitCastOperator template
 *
 * This static instance registers the ExplicitCastOperator with the
 * TemplateRegistry during static initialization (before main() starts).
 */
TemplateRegistrar<ExplicitCastOperator> register_explicit_cast_operator;

} // anonymous namespace

}} // namespace wjh::atlas::generation::v1
