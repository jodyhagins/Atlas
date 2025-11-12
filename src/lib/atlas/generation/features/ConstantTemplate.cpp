// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "ConstantTemplate.hpp"

#include "atlas/generation/core/ClassInfo.hpp"
#include "atlas/generation/core/TemplateRegistry.hpp"

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

// Self-registration with the template registry
namespace {
TemplateRegistrar<ConstantDeclarationsTemplate> constant_declarations_registrar;
TemplateRegistrar<ConstantDefinitionsTemplate> constant_definitions_registrar;
}

} // namespace wjh::atlas::generation
