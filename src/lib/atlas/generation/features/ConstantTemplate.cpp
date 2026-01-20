// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "ConstantTemplate.hpp"

#include "atlas/generation/core/ClassInfo.hpp"
#include "atlas/generation/core/TemplateRegistry.hpp"

#include <boost/mustache.hpp>

#include <sstream>

namespace wjh::atlas::generation {

// ConstantDeclarationsTemplate - for inside the class body
std::string_view
ConstantDeclarationsTemplate::
get_template_impl() const noexcept
{
    // Use 'static const' declaration because type is incomplete at this point
    static constexpr std::string_view tmpl = R"(
    static const {{{class_name}}} {{{name}}};
)";
    return tmpl;
}

bool
ConstantDeclarationsTemplate::
should_apply_impl(ClassInfo const & info) const
{
    return not info.constants.empty();
}

boost::json::object
ConstantDeclarationsTemplate::
prepare_variables_impl(ClassInfo const & info) const
{
    boost::json::object variables;
    variables["class_name"] = info.class_name;
    return variables;
}

// ConstantDefinitionsTemplate - for outside the class body
std::string_view
ConstantDefinitionsTemplate::
get_template_impl() const noexcept
{
    // This is standard-compliant: declare as 'const', define as 'constexpr'
    // Note: The pragma wrapper is added in render_impl() around ALL constants
    static constexpr std::string_view tmpl =
        R"(inline {{{const_qualifier}}}{{{full_qualified_name}}} {{{full_qualified_name}}}::{{{name}}} = {{{full_qualified_name}}}({{{value}}});
)";
    return tmpl;
}

bool
ConstantDefinitionsTemplate::
should_apply_impl(ClassInfo const & info) const
{
    return not info.constants.empty();
}

boost::json::object
ConstantDefinitionsTemplate::
prepare_variables_impl(ClassInfo const & info) const
{
    boost::json::object variables;

    variables["const_qualifier"] = "constexpr ";
    variables["full_qualified_name"] = info.full_qualified_name;
    return variables;
}

std::string
ConstantDefinitionsTemplate::
render_impl(ClassInfo const & info) const
{
    if (info.constants.empty()) {
        return "";
    }

    std::ostringstream oss;

    // Leading newline to separate from class body
    oss << "\n";

    // Output pragma push to disable clang warnings for global constructors
    // and exit-time destructors (relevant for non-trivial types like std::string)
    oss << "#if defined(__clang__)\n"
        << "#pragma clang diagnostic push\n"
        << "#pragma clang diagnostic ignored \"-Wexit-time-destructors\"\n"
        << "#pragma clang diagnostic ignored \"-Wglobal-constructors\"\n"
        << "#endif\n";

    // Get the template string for individual constants
    std::string_view tmpl_str = get_template();

    // Render each constant
    for (auto const & constant : info.constants) {
        boost::json::object vars = prepare_variables(info);
        vars["name"] = constant.name;
        vars["value"] = constant.value;
        vars["const_qualifier"] = info.const_qualifier;

        boost::mustache::render(tmpl_str, oss, vars, boost::json::object{});
    }

    // Output pragma pop
    oss << "#if defined(__clang__)\n"
        << "#pragma clang diagnostic pop\n"
        << "#endif\n";

    return oss.str();
}

// Self-registration with the template registry
namespace {
TemplateRegistrar<ConstantDeclarationsTemplate> constant_declarations_registrar;
TemplateRegistrar<ConstantDefinitionsTemplate> constant_definitions_registrar;
}

} // namespace wjh::atlas::generation
