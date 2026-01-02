// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "ProfileSystem.hpp"
#include "TemplateSystem.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace wjh::atlas {

bool
TemplateSystem::
is_valid_template_name(std::string const & name)
{
    if (name.empty()) {
        return false;
    }

    // Must start with letter or underscore
    if (not std::isalpha(static_cast<unsigned char>(name[0])) && name[0] != '_')
    {
        return false;
    }

    // Rest must be alphanumeric or underscore
    for (char c : name) {
        if (not std::isalnum(static_cast<unsigned char>(c)) && c != '_') {
            return false;
        }
    }

    return true;
}

bool
TemplateSystem::
is_valid_parameter_name(std::string const & name)
{
    // Same rules as template name
    return is_valid_template_name(name);
}

void
TemplateSystem::
register_template(
    TypeTemplate const & tmpl,
    ProfileSystem const & profile_system)
{
    if (not is_valid_template_name(tmpl.name)) {
        throw std::runtime_error(
            "Invalid template name: '" + tmpl.name +
            "'. Template names must be valid C++ identifiers");
    }

    if (templates_.find(tmpl.name) != templates_.end()) {
        throw std::runtime_error(
            "Template '" + tmpl.name + "' is already registered");
    }

    if (tmpl.parameters.empty()) {
        throw std::runtime_error(
            "Template '" + tmpl.name + "' must have at least one parameter");
    }

    // Validate each parameter
    for (auto const & param : tmpl.parameters) {
        if (not is_valid_parameter_name(param)) {
            throw std::runtime_error(
                "Invalid parameter name '" + param + "' in template '" +
                tmpl.name + "'. Parameter names must be valid C++ identifiers");
        }

        // Check for collision with profile names
        if (profile_system.has_profile(param)) {
            throw std::runtime_error(
                "Template parameter '" + param + "' in template '" + tmpl.name +
                "' conflicts with an existing profile of the same name");
        }
    }

    // Check for duplicate parameters
    std::vector<std::string> sorted_params = tmpl.parameters;
    std::sort(sorted_params.begin(), sorted_params.end());
    auto dup = std::adjacent_find(sorted_params.begin(), sorted_params.end());
    if (dup != sorted_params.end()) {
        throw std::runtime_error(
            "Duplicate parameter name '" + *dup + "' in template '" +
            tmpl.name + "'");
    }

    templates_[tmpl.name] = tmpl;
}

bool
TemplateSystem::
has_template(std::string const & name) const
{
    return templates_.find(name) != templates_.end();
}

TypeTemplate const &
TemplateSystem::
get_template(std::string const & name) const
{
    auto it = templates_.find(name);
    if (it == templates_.end()) {
        throw std::runtime_error(
            "Unknown template: '" + name +
            "'. Template must be defined before use");
    }
    return it->second;
}

std::vector<std::string>
TemplateSystem::
get_template_names() const
{
    std::vector<std::string> names;
    names.reserve(templates_.size());
    for (auto const & pair : templates_) {
        names.push_back(pair.first);
    }
    std::sort(names.begin(), names.end());
    return names;
}

void
TemplateSystem::
clear()
{
    templates_.clear();
}

std::string
substitute_template_params(
    std::string const & input,
    std::vector<std::string> const & parameters,
    std::vector<std::string> const & arguments)
{
    if (parameters.size() != arguments.size()) {
        throw std::runtime_error(
            "Template parameter count (" + std::to_string(parameters.size()) +
            ") does not match argument count (" +
            std::to_string(arguments.size()) + ")");
    }

    std::string result = input;

    // Replace each {PARAM} with its corresponding argument
    for (size_t i = 0; i < parameters.size(); ++i) {
        std::string placeholder = "{" + parameters[i] + "}";
        std::string const & replacement = arguments[i];

        size_t pos = 0;
        while ((pos = result.find(placeholder, pos)) != std::string::npos) {
            result.replace(pos, placeholder.length(), replacement);
            pos += replacement.length();
        }
    }

    return result;
}

} // namespace wjh::atlas
