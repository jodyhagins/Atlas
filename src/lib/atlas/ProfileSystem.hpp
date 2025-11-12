// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_B4F2E8A1C9D64E7FA2B5C3D8E1F4A7B2
#define WJH_ATLAS_B4F2E8A1C9D64E7FA2B5C3D8E1F4A7B2

#include "AtlasUtilities.hpp"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace wjh::atlas {

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
     * @brief Register a profile with its parsed specification
     *
     * @param name Profile name (must be [a-zA-Z0-9_-]+)
     * @param spec Parsed specification containing forwards and operators
     * @throws std::runtime_error if name is invalid or already exists
     */
    void register_profile(
        std::string const & name,
        ParsedSpecification const & spec);

    /**
     * @brief Check if a profile exists
     */
    bool has_profile(std::string const & name) const;

    /**
     * @brief Get a profile's parsed specification
     * @throws std::runtime_error if profile doesn't exist
     */
    ParsedSpecification const & get_profile(std::string const & name) const;

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

    std::unordered_map<std::string, ParsedSpecification> profiles_;
};

} // namespace wjh::atlas

#endif // WJH_ATLAS_B4F2E8A1C9D64E7FA2B5C3D8E1F4A7B2
