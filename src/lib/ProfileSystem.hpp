// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_B4F2E8A1C9D64E7FA2B5C3D8E1F4A7B2
#define WJH_ATLAS_B4F2E8A1C9D64E7FA2B5C3D8E1F4A7B2

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace wjh::atlas { inline namespace v1 {

/**
 * @brief Simple profile system for user-defined feature bundles
 *
 * Profiles are named collections of features defined by the user:
 *   profile=NUMERIC; +, -, *, /, ==, !=, hash
 *
 * Used in descriptions via text substitution:
 *   description=strong double; {NUMERIC}, ->
 *
 * Multiple profiles can be composed:
 *   description=strong int; {NUMERIC}, {BITWISE}, %
 */
class ProfileSystem
{
public:
    /**
     * @brief Register a profile with its features
     *
     * @param name Profile name (must be [a-zA-Z0-9_-]+)
     * @param features Vector of feature strings
     * @throws std::runtime_error if name is invalid or already exists
     */
    void register_profile(
        std::string const & name,
        std::vector<std::string> const & features);

    /**
     * @brief Expand {NAME} tokens in feature list
     *
     * Performs text substitution of {NAME} with profile features.
     * Returns deduplicated list of features.
     *
     * @param input_features Features that may contain {NAME} tokens
     * @return Expanded and deduplicated feature list
     * @throws std::runtime_error if referenced profile doesn't exist
     */
    std::vector<std::string> expand_features(
        std::vector<std::string> const & input_features) const;

    /**
     * @brief Check if a profile exists
     */
    bool has_profile(std::string const & name) const;

    /**
     * @brief Get all registered profile names
     */
    std::vector<std::string> get_profile_names() const;

    /**
     * @brief Clear all profiles (useful for testing)
     */
    void clear();

private:
    /**
     * @brief Validate profile name
     * @return true if name matches [a-zA-Z0-9_-]+
     */
    static bool is_valid_profile_name(std::string const & name);

    std::unordered_map<std::string, std::vector<std::string>> profiles_;
};

}} // namespace wjh::atlas::v1

#endif // WJH_ATLAS_B4F2E8A1C9D64E7FA2B5C3D8E1F4A7B2
