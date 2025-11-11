#pragma once

#include "atlas/generation/core/ClassInfo.hpp"
#include "atlas/generation/core/ITemplate.hpp"

namespace wjh::atlas::generation { inline namespace v1 {

/**
 * @brief Template class for generating explicit cast operators
 *
 * Generates explicit operator T() for all user-specified types in the
 * explicit_cast_operators vector.
 *
 * This template iterates internally over all cast types, rendering each one
 * and accumulating the results. This prevents duplicate generation when the
 * MainTemplate iterates over the cast array.
 *
 * Example generated code:
 * @code
 *     explicit operator int() const
 *     noexcept(noexcept(static_cast<int>(
 *         std::declval<double const&>())))
 *     {
 *         return static_cast<int>(value);
 *     }
 * @endcode
 */
class ExplicitCastOperator final
: public ITemplate
{
public:
    /**
     * @brief Default constructor
     *
     * This template handles all explicit casts in a single instance,
     * iterating over ClassInfo::explicit_cast_operators internally.
     */
    ExplicitCastOperator() = default;

protected:
    [[nodiscard]]
    std::string id_impl() const override;

    [[nodiscard]]
    std::string_view get_template_impl() const override;

    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;

    [[nodiscard]]
    std::string render_impl(ClassInfo const & info) const override;

private:
    /**
     * @brief Prepare variables for a specific cast type
     * @param info ClassInfo containing type metadata
     * @param cast CastOperator containing the target cast type
     * @return JSON object with variables for this specific cast
     */
    [[nodiscard]]
    boost::json::object prepare_variables_for_cast(
        ClassInfo const & info,
        CastOperator const & cast) const;
};

}} // namespace wjh::atlas::generation::v1
