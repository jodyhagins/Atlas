// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_6F9E8D7C6B5A4D3E2F1A0B9C8D7E6F5E
#define WJH_ATLAS_6F9E8D7C6B5A4D3E2F1A0B9C8D7E6F5E

#include "atlas/generation/core/ITemplate.hpp"

namespace wjh::atlas::generation {

/**
 * Address-of operator template
 *
 * Generates the unary address-of operator (operator&) for a strong type:
 * - Returns a pointer to the wrapped value
 * - Provides both const and non-const overloads
 * - Both overloads are noexcept
 * - Uses std::addressof to bypass overloaded operator&
 *
 * The generated operator allows taking the address of the underlying value
 * through the strong type wrapper.
 *
 * Example:
 * @code
 * UserId id{42};
 * int* ptr = &id;  // Returns: pointer to the underlying int
 * @endcode
 *
 * Design notes:
 * - Uses std::addressof to ensure correct behavior even if the underlying
 *   type has an overloaded operator&
 * - Both overloads are unconditionally noexcept
 * - Requires <memory> header for std::addressof
 * - This operator is triggered by the "&of" token in the description
 */
class AddressOfOperator final
: public ITemplate
{
protected:
    [[nodiscard]]
    std::string id_impl() const override
    {
        return "operators.functional.addressof";
    }

    [[nodiscard]]
    std::string_view get_template_impl() const noexcept override;

    /**
     * Check if address-of operator is enabled
     *
     * Examines the ClassInfo to determine if the "&of" operator
     * has been requested for this strong type.
     *
     * @param info Strong type class information
     * @return true if address-of operator is enabled
     */
    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;

    /**
     * Prepare variables for address-of operator rendering
     *
     * Creates a JSON object with variables needed for rendering:
     * - const_expr: constexpr specifier if applicable
     * - underlying_type: the wrapped type
     * - op: the operator symbol ("&")
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
        return {"<memory>"};
    }
};

} // namespace wjh::atlas::generation

#endif // WJH_ATLAS_6F9E8D7C6B5A4D3E2F1A0B9C8D7E6F5E
