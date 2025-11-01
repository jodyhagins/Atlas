// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "ClassInfo.hpp"
#include "TemplateRegistry.hpp"

#include <boost/mustache.hpp>

#include <sstream>
#include <stdexcept>

namespace wjh::atlas::generation { inline namespace v1 {

// ============================================================================
// TemplateRegistry Implementation
// ============================================================================

TemplateRegistry &
TemplateRegistry::
instance()
{
    static TemplateRegistry registry;
    return registry;
}

void
TemplateRegistry::
register_template(std::unique_ptr<ITemplate> tmpl)
{
    if (not tmpl) [[unlikely]] {
        throw std::runtime_error(
            "TemplateRegistry::register_template: null template pointer");
    }

    std::string id = tmpl->id();
    if (id.empty()) [[unlikely]] {
        throw std::runtime_error(
            "TemplateRegistry::register_template: template has empty ID");
    }

    auto [iter, inserted] = templates_.try_emplace(id, std::move(tmpl));
    if (not inserted) [[unlikely]] {
        std::stringstream strm;
        strm << "TemplateRegistry::register_template: duplicate template ID: "
            << id;
        throw std::runtime_error(strm.str());
    }
}

ITemplate const *
TemplateRegistry::
get_template(std::string_view id) const
{
    auto iter = templates_.find(id);
    return (iter != templates_.end()) ? iter->second.get() : nullptr;
}

bool
TemplateRegistry::
has_template(std::string_view id) const
{
    return templates_.find(id) != templates_.end();
}

void
TemplateRegistry::
clear()
{
    templates_.clear();
}

// ============================================================================
// ITemplate Default Implementation
// ============================================================================

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
