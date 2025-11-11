// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "ClassInfo.hpp"
#include "ITemplate.hpp"

#include <boost/mustache.hpp>

#include <sstream>
#include <stdexcept>

namespace wjh::atlas::generation { inline namespace v1 {

// ============================================================================
// ITemplate Implementation
// ============================================================================

ITemplate::
~ITemplate() noexcept = default;

std::string
ITemplate::
id() const
{
    return id_impl();
}

std::string
ITemplate::
sort_key() const
{
    return sort_key_impl();
}

std::string_view
ITemplate::
get_template() const
{
    return get_template_impl();
}

bool
ITemplate::
should_apply(ClassInfo const & info) const
{
    return should_apply_impl(info);
}

boost::json::object
ITemplate::
prepare_variables(ClassInfo const & info) const
{
    // Get template-specific variables from derived class
    auto vars = prepare_variables_impl(info);

    // Add common variables if not already set by the impl
    // This allows derived classes to override these if needed
    if (not vars.contains("value")) {
        vars["value"] = info.value_member_name;
    }
    if (not vars.contains("const_expr")) {
        vars["const_expr"] = info.const_expr;
    }
    if (not vars.contains("class_name")) {
        vars["class_name"] = info.class_name;
    }
    if (not vars.contains("underlying_type")) {
        vars["underlying_type"] = info.underlying_type;
    }
    if (not vars.contains("full_qualified_name")) {
        vars["full_qualified_name"] = info.full_qualified_name;
    }
    if (not vars.contains("has_constraint")) {
        vars["has_constraint"] = info.has_constraint &&
            not info.constraint_type.empty();
    }
    if (not vars.contains("constraint_message") && info.has_constraint) {
        vars["constraint_message"] = info.constraint_message;
    }

    return vars;
}

std::set<std::string>
ITemplate::
required_includes() const
{
    return required_includes_impl();
}

std::set<std::string>
ITemplate::
required_preamble() const
{
    return required_preamble_impl();
}

void
ITemplate::
validate(ClassInfo const & info) const
{
    validate_impl(info);
}

std::string
ITemplate::
render(ClassInfo const & info) const
{
    return render_impl(info);
}

boost::json::object
ITemplate::
prepare_variables_impl(ClassInfo const &) const
{
    return {};
}

std::string
ITemplate::
sort_key_impl() const
{
    return id_impl();
}

std::set<std::string>
ITemplate::
required_includes_impl() const
{
    return {};
}

std::set<std::string>
ITemplate::
required_preamble_impl() const
{
    return {};
}

void
ITemplate::
validate_impl(ClassInfo const &) const
{
    // Default: no validation required
}

std::string
ITemplate::
render_impl(ClassInfo const & info) const
{
    // Validate that this template should be applied
    validate(info);

    if (not should_apply(info)) [[unlikely]] {
        std::stringstream strm;
        strm << "ITemplate::render: template '" << id()
            << "' should not apply to type '" << info.desc.type_name << "'";
        throw std::runtime_error(strm.str());
    }

    // Prepare variables for rendering
    boost::json::object variables = prepare_variables(info);

    // Get the template string
    std::string_view tmpl_str = get_template();
    if (tmpl_str.empty()) {
        // Empty template is valid - just return empty string
        return "";
    }

    // Render using Mustache
    try {
        std::ostringstream oss;
        boost::mustache::render(
            tmpl_str,
            oss,
            variables,
            boost::json::object{});
        return oss.str();
    } catch (std::exception const & e) {
        std::stringstream strm;
        strm << "ITemplate::render: Mustache rendering failed for template '"
            << id() << "': " << e.what();
        throw std::runtime_error(strm.str());
    }
}

}} // namespace wjh::atlas::generation::v1
