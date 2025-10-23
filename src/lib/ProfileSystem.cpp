// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#include "ProfileSystem.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace wjh::atlas { inline namespace v1 {

bool
ProfileSystem::
is_valid_profile_name(std::string const & name)
{
    if (name.empty()) {
        return false;
    }

    for (char c : name) {
        if (not std::isalnum(static_cast<unsigned char>(c)) && c != '_' &&
            c != '-')
        {
            return false;
        }
    }

    return true;
}

void
ProfileSystem::
register_profile(std::string const & name, ParsedSpecification const & spec)
{
    if (not is_valid_profile_name(name)) {
        throw std::runtime_error(
            "Invalid profile name: '" + name +
            "'. "
            "Profile names must match [a-zA-Z0-9_-]+");
    }

    if (profiles_.find(name) != profiles_.end()) {
        throw std::runtime_error(
            "Profile '" + name + "' is already registered");
    }

    profiles_[name] = spec;
}

bool
ProfileSystem::
has_profile(std::string const & name) const
{
    return profiles_.find(name) != profiles_.end();
}

ParsedSpecification const &
ProfileSystem::
get_profile(std::string const & name) const
{
    auto it = profiles_.find(name);
    if (it == profiles_.end()) {
        throw std::runtime_error(
            "Unknown profile: '" + name +
            "'. "
            "Profile must be defined before use");
    }
    return it->second;
}

std::vector<std::string>
ProfileSystem::
get_profile_names() const
{
    std::vector<std::string> names;
    names.reserve(profiles_.size());
    for (auto const & pair : profiles_) {
        names.push_back(pair.first);
    }
    std::sort(names.begin(), names.end());
    return names;
}

void
ProfileSystem::
clear()
{
    profiles_.clear();
}

}} // namespace wjh::atlas::v1
