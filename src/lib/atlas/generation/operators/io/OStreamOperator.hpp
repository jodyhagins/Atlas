// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_E9F8D7C6B5A4D3E2F1A0B9C8D7E6F5A4
#define WJH_ATLAS_E9F8D7C6B5A4D3E2F1A0B9C8D7E6F5A4

#include "atlas/generation/core/ITemplate.hpp"

namespace wjh::atlas::generation {

/**
 * Output stream operator template
 *
 * Generates the stream insertion operator (<<) for a strong type:
 * - Inserts the wrapped value into an std::ostream
 * - Returns the ostream for chaining
 * - Uses friend function for proper ADL
 *
 * The generated operator allows strong types to be used with standard
 * output streams like std::cout, std::ostringstream, etc.
 *
 * Example:
 * @code
 * UserId id{42};
 * std::cout << id;  // Outputs: 42
 * @endcode
 *
 * Design notes:
 * - The operator forwards directly to the underlying value's operator<<
 * - No formatting or decoration is added by the strong type wrapper
 * - Requires <ostream> header to be included
 */
class OStreamOperator final
: public ITemplate
{
protected:
    [[nodiscard]]
    std::string id_impl() const override
    {
        return "operators.io.ostream";
    }

    /**
     * Sort key for output stream operator
     *
     * @return Sort key: "<<" (the output stream operator symbol)
     */
    [[nodiscard]]
    std::string sort_key_impl() const override
    {
        return "<<";
    }

    [[nodiscard]]
    std::string_view get_template_impl() const noexcept override;

    /**
     * Check if output stream operator is enabled
     *
     * Examines the ClassInfo to determine if the "out" operator
     * has been requested for this strong type.
     *
     * @param info Strong type class information
     * @return true if ostream operator is enabled
     */
    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;

    /**
     * Prepare variables for ostream operator rendering
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
        return {"<ostream>"};
    }
};

} // namespace wjh::atlas::generation

#endif // WJH_ATLAS_E9F8D7C6B5A4D3E2F1A0B9C8D7E6F5A4
