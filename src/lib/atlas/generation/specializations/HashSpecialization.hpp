#pragma once

#include "atlas/generation/core/ITemplate.hpp"

namespace wjh::atlas::generation {

/**
 * @brief Template class for generating std::hash specialization
 *
 * Generates a std::hash specialization that enables use of the strong type
 * in hash-based containers like std::unordered_map and std::unordered_set.
 * The specialization delegates hashing to std::hash of the underlying type.
 *
 * The hash function is conditionally noexcept based on whether the underlying
 * type's hash function is noexcept. It can optionally be marked constexpr
 * (controlled by hash_const_expr in ClassInfo).
 *
 * Example generated code:
 * @code
 * template <>
 * struct std::hash<MyNamespace::MyType>
 * {
 *     ATLAS_NODISCARD
 *     constexpr std::size_t operator () (MyNamespace::MyType const & t) const
 *     noexcept(
 *         noexcept(std::hash<int>{}(
 *             std::declval<int const &>())))
 *     {
 *         return std::hash<int>{}(
 *             static_cast<int const &>(t));
 *     }
 * };
 * @endcode
 *
 * @note This specialization is placed outside the type's namespace,
 *       in namespace std, as required for specializing standard library
 * templates.
 * @note Requires <functional> header for std::hash
 */
class HashSpecialization final
: public ITemplate
{
protected:
    [[nodiscard]]
    std::string id_impl() const override;

    [[nodiscard]]
    std::string_view get_template_impl() const override;

    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;

    [[nodiscard]]
    boost::json::object prepare_variables_impl(
        ClassInfo const & info) const override;
};

} // namespace wjh::atlas::generation
