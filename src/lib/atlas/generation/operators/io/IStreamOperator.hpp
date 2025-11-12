// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_F0E9D8C7B6A5D4E3F2A1B0C9D8E7F6A5
#define WJH_ATLAS_F0E9D8C7B6A5D4E3F2A1B0C9D8E7F6A5

#include "atlas/generation/core/ITemplate.hpp"

namespace wjh::atlas::generation {

/**
 * Input stream operator template
 *
 * Generates the stream extraction operator (>>) for a strong type:
 * - Extracts a value from an std::istream into the wrapped object
 * - Returns the istream for chaining
 * - Uses friend function for proper ADL
 *
 * The generated operator allows strong types to be used with standard
 * input streams like std::cin, std::istringstream, etc.
 *
 * Example:
 * @code
 * UserId id;
 * std::cin >> id;  // Reads value into wrapped integer
 * @endcode
 *
 * Design notes:
 * - The operator forwards directly to the underlying value's operator>>
 * - No parsing or validation is added by the strong type wrapper
 * - Constraint validation (if any) is not automatically invoked
 * - Requires <istream> header to be included
 * - Takes non-const reference to allow modification
 */
class IStreamOperator final
: public ITemplate
{
protected:
    [[nodiscard]]
    std::string id_impl() const override
    {
        return "operators.io.istream";
    }

    /**
     * Sort key for input stream operator
     *
     * @return Sort key: ">>" (the input stream operator symbol)
     */
    [[nodiscard]]
    std::string sort_key_impl() const override
    {
        return ">>";
    }

    [[nodiscard]]
    std::string_view get_template_impl() const noexcept override;

    /**
     * Check if input stream operator is enabled
     *
     * Examines the ClassInfo to determine if the "in" operator
     * has been requested for this strong type.
     *
     * @param info Strong type class information
     * @return true if istream operator is enabled
     */
    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;

    /**
     * Prepare variables for istream operator rendering
     *
     * Creates a JSON object with variables needed for rendering:
     * - class_name: name of the strong type class
     *
     * @param info Strong type class information
     * @return JSON object with template variables
     */
    [[nodiscard]]
    boost::json::object prepare_variables_impl(
        ClassInfo const & info) const override;

    /**
     * Specify required includes
     *
     * @return Set containing required headers
     */
    [[nodiscard]]
    std::set<std::string> required_includes_impl() const override
    {
        return {"<istream>"};
    }
};

} // namespace wjh::atlas::generation

#endif // WJH_ATLAS_F0E9D8C7B6A5D4E3F2A1B0C9D8E7F6A5
