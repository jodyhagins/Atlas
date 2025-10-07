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
register_profile(
    std::string const & name,
    std::vector<std::string> const & features)
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

    profiles_[name] = features;
}

std::vector<std::string>
ProfileSystem::
expand_features(std::vector<std::string> const & input_features) const
{
    std::unordered_set<std::string> result_set;
    std::vector<std::string> result;

    for (auto const & feature : input_features) {
        // Check if this is a profile reference: {NAME}
        if (feature.length() > 2 && feature[0] == '{' &&
            feature[feature.length() - 1] == '}')
        {
            // Extract profile name
            std::string profile_name = feature.substr(1, feature.length() - 2);

            // Look up profile
            auto it = profiles_.find(profile_name);
            if (it == profiles_.end()) {
                throw std::runtime_error(
                    "Unknown profile: '{" + profile_name +
                    "}'. "
                    "Profile must be defined with 'profile=" +
                    profile_name + "; ...' before use");
            }

            // Add all features from profile (text substitution)
            for (auto const & profile_feature : it->second) {
                if (result_set.insert(profile_feature).second) {
                    result.push_back(profile_feature);
                }
            }
        } else {
            // Regular feature
            if (result_set.insert(feature).second) {
                result.push_back(feature);
            }
        }
    }

    // Sort for deterministic output (makes diffs easier)
    std::sort(result.begin(), result.end());

    return result;
}

bool
ProfileSystem::
has_profile(std::string const & name) const
{
    return profiles_.find(name) != profiles_.end();
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
