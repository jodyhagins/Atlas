// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_4F9E8D7C6B5A4D3E2F1A0B9C8D7E6F5C
#define WJH_ATLAS_4F9E8D7C6B5A4D3E2F1A0B9C8D7E6F5C

#include "atlas/generation/core/ITemplate.hpp"

namespace wjh::atlas::generation {

/**
 * Callable operator template
 *
 * Generates a call operator that takes an invocable (operator(InvocableT&&))
 * for a strong type:
 * - Accepts any invocable (function, lambda, function object, etc.)
 * - Invokes the provided function with the wrapped value
 * - Returns the result of the invocation
 * - Provides both const and non-const overloads
 * - Conditionally noexcept based on the invocable
 * - Uses std::invoke when available (C++17+) for maximum flexibility
 *
 * The generated operator allows strong types to be used with functional
 * programming patterns like map, transform, and composition.
 *
 * Example:
 * @code
 * UserId id{42};
 * auto str = id([](int i) { return std::to_string(i); });  // Returns: "42"
 * @endcode
 *
 * Design notes:
 * - Uses std::invoke when __cpp_lib_invoke >= 201411L for better compatibility
 * - Falls back to direct invocation for earlier compilers
 * - Perfect forwarding preserves value category of the invocable
 * - Noexcept specification propagates from the invocable
 * - Requires <utility> and <functional> headers
 */
class CallableOperator final
: public ITemplate
{
protected:
    [[nodiscard]]
    std::string id_impl() const override
    {
        return "operators.functional.callable";
    }

    [[nodiscard]]
    std::string_view get_template_impl() const noexcept override;

    /**
     * Check if callable operator is enabled
     *
     * Examines the ClassInfo to determine if the "(&)" operator
     * has been requested for this strong type.
     *
     * @param info Strong type class information
     * @return true if callable operator is enabled
     */
    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;

    /**
     * Specify required includes
     *
     * @return Set containing required headers
     */
    [[nodiscard]]
    std::set<std::string> required_includes_impl() const override
    {
        return {"<utility>", "<functional>"};
    }
};

} // namespace wjh::atlas::generation

#endif // WJH_ATLAS_4F9E8D7C6B5A4D3E2F1A0B9C8D7E6F5C
