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

}} // namespace wjh::atlas::generation::v1
