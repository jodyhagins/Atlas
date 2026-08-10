// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_A3F7B2C8D1E4F6A9B5C2D8E3F0A1B4C7
#define WJH_ATLAS_A3F7B2C8D1E4F6A9B5C2D8E3F0A1B4C7

#include "atlas/generation/core/ITemplate.hpp"

namespace wjh::atlas::generation {

/**
 * From-atlas-cast constructor
 *
 * Generates a constructor that accepts a listed atlas type (or a type that
 * drills to one) and constructs via atlas::cast drilling. The allowlist is
 * the set of type names from from_atlas_cast<A, B, ...> plus, under
 * auto_from_atlas_cast=true, the other types declared in the same file.
 *
 * SFINAE guards:
 * 1. has_atlas_value_type - only atlas types (non-atlas handled by variadic ctor)
 * 2. !is_same - don't shadow copy/move constructors
 * 3. !is_constructible<underlying_type, ArgT&&> - don't compete with variadic ctor
 * 4. disjunction of is_atlas_castable<ArgT, Named> - the argument is, or
 *    drills to, a listed type (reference probe, so the named type must be an
 *    actual layer of the argument's chain)
 * 5. is_from_cast_constructible - the exact cast the body performs is valid
 *    for the argument's value category, so borrow-storing constructions from
 *    temporaries SFINAE out instead of hard-erroring
 */
class FromAtlasCastConstructor final
: public ITemplate
{
protected:
    [[nodiscard]]
    std::string id_impl() const override
    {
        return "features.from_atlas_cast_constructor";
    }

    [[nodiscard]]
    std::string_view get_template_impl() const noexcept override;

    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;

    [[nodiscard]]
    boost::json::object prepare_variables_impl(
        ClassInfo const & info) const override;
};

} // namespace wjh::atlas::generation

#endif // WJH_ATLAS_A3F7B2C8D1E4F6A9B5C2D8E3F0A1B4C7
