#include "BoolOperator.hpp"

#include "atlas/generation/core/ClassInfo.hpp"
#include "atlas/generation/core/TemplateRegistry.hpp"

namespace wjh::atlas::generation { inline namespace v1 {

std::string
BoolOperator::
id_impl() const
{
    return "operators.conversion.bool";
}

std::string_view
BoolOperator::
get_template_impl() const
{
    static constexpr std::string_view tmpl = R"(
    /**
     * Return the result of casting the wrapped object to bool.
     */
    {{{const_expr}}}explicit operator bool () const
    noexcept(noexcept(static_cast<bool>(
        std::declval<{{{underlying_type}}} const&>())))
    {
        return static_cast<bool>(value);
    }
)";
    return tmpl;
}

bool
BoolOperator::
should_apply_impl(ClassInfo const & info) const
{
    return info.bool_operator;
}

// Self-registration with the template registry
namespace {
TemplateRegistrar<BoolOperator> bool_operator_registrar;
}

}} // namespace wjh::atlas::generation::v1
