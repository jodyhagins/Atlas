#pragma once

#include "atlas/generation/core/ITemplate.hpp"

namespace wjh::atlas::generation {

/**
 * @brief Template class for generating std::formatter specialization
 *
 * Generates a std::formatter specialization that enables use of the strong type
 * with std::format and std::print in C++20 and later. The specialization
 * inherits from std::formatter of the underlying type and delegates all
 * formatting operations.
 *
 * The generated specialization is wrapped in a feature test macro check
 * (__cpp_lib_format >= 202110L) to ensure compatibility with pre-C++20
 * compilers or standard libraries that don't yet implement std::format.
 *
 * Example generated code:
 * @code
 * #if defined(__cpp_lib_format) && __cpp_lib_format >= 202110L
 * template <>
 * struct std::formatter<MyNamespace::MyType> : std::formatter<int>
 * {
 *     auto format(MyNamespace::MyType const & t, std::format_context & ctx)
 * const
 *     {
 *         return std::formatter<int>::format(
 *             static_cast<int const &>(t), ctx);
 *     }
 * };
 * #endif // defined(__cpp_lib_format) && __cpp_lib_format >= 202110L
 * @endcode
 *
 * Usage example:
 * @code
 * MyType value{42};
 * std::string s = std::format("{}", value);  // Works with the specialization
 * std::print("{}\n", value);                  // Also works
 * @endcode
 *
 * @note This specialization is placed outside the type's namespace,
 *       in namespace std, as required for specializing standard library
 * templates.
 * @note The formatter inherits format specification support from the underlying
 * type
 * @note Requires C++20 and <format> header
 */
class FormatterSpecialization final
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
