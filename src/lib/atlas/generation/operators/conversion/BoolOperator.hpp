#pragma once

#include "atlas/generation/core/ITemplate.hpp"

namespace wjh::atlas::generation {

/**
 * @brief Template class for generating bool conversion operator
 *
 * Generates an explicit operator bool() that converts the strong type
 * to bool by casting the underlying value.
 *
 * Example generated code:
 * @code
 *     explicit operator bool () const
 *     noexcept(noexcept(static_cast<bool>(
 *         std::declval<int const&>())))
 *     {
 *         return static_cast<bool>(value);
 *     }
 * @endcode
 */
class BoolOperator final
: public ITemplate
{
protected:
    [[nodiscard]]
    std::string id_impl() const override;

    [[nodiscard]]
    std::string_view get_template_impl() const override;

    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;

    /**
     * Render the operator inside a -Wfloat-equal suppression
     *
     * GCC reads the cast of a floating-point value to bool as a comparison
     * against zero and warns under -Wfloat-equal. The user cannot silence a
     * warning raised inside generated code, so the generator does it.
     *
     * @param info Strong type class information
     * @return Rendered operator wrapped in diagnostic push/pop
     */
    [[nodiscard]]
    std::string render_impl(ClassInfo const & info) const override;
};

} // namespace wjh::atlas::generation
