// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_7A3F9E2C8B4D5F6A1C9E7B3D8A5F2C4E
#define WJH_ATLAS_7A3F9E2C8B4D5F6A1C9E7B3D8A5F2C4E

#include <boost/json.hpp>

#include <set>
#include <string>
#include <string_view>

namespace wjh::atlas::generation { inline namespace v1 {

// Forward declaration
struct ClassInfo;

}} // namespace wjh::atlas::generation::v1

namespace wjh::atlas::generation { inline namespace v1 {

/**
 * Base interface for all template classes using Non-Virtual Interface (NVI)
 * pattern
 *
 * This interface defines the contract for code generation templates in the
 * StrongTypeGenerator refactoring. Each template is responsible for generating
 * a specific piece of functionality (operators, features, specializations,
 * etc.).
 *
 * The NVI pattern is employed here:
 * - Public member functions are non-virtual and provide the stable interface
 * - Protected virtual *_impl() member functions allow customization by derived
 * classes
 * - This ensures consistent pre/post-conditions while enabling flexibility
 *
 * Templates are self-registering via the TemplateRegistrar helper and
 * TemplateRegistry singleton.
 *
 * Example usage:
 * @code
 * class ArithmeticTemplate : public ITemplate {
 * protected:
 *     std::string id_impl() const override {
 *         return "arithmetic.addition";
 *     }
 *
 *     bool should_apply_impl(ClassInfo const& info) const override {
 *         return info.has_operator("+");
 *     }
 *
 *     // ... implement other _impl() member functions
 * };
 *
 * // Self-registration
 * namespace {
 *     TemplateRegistrar<ArithmeticTemplate> registrar;
 * }
 * @endcode
 */
class ITemplate
{
public:
    virtual ~ITemplate() noexcept;

    /**
     * Get unique identifier for this template
     *
     * Returns a hierarchical identifier (e.g., "operators.arithmetic.addition")
     * used for registration and diagnostics.
     *
     * @return Unique string identifier
     */
    [[nodiscard]]
    std::string id() const;

    /**
     * Get the sort key for this template
     *
     * Returns a string used to determine the order in which templates are
     * visited during code generation. This is particularly important for
     * operators, which should be sorted by their operator symbol rather than
     * their template ID.
     *
     * For operator templates, this should return the operator symbol:
     * - Arithmetic: "+", "-", "*", "/", "%"
     * - Comparison: "==", "!=", "<", "<=", ">", ">=", "<=>"
     * - Logical: "!", "&&", "||"
     * - I/O: "<<", ">>"
     * - Access: "->", "*" (indirection)
     * - Functional: "()", "[]", "&" (address-of)
     * - etc.
     *
     * For non-operator templates, this typically returns the template ID
     * or a descriptive key that ensures appropriate ordering:
     * - Features: "constants", "forward", "iterable", etc.
     * - Specializations: "hash", "fmt"
     * - Core: "" (to ensure main template sorts first)
     *
     * Note: Templates handling multiple operator variants (like arithmetic
     * operators with checked/saturating/wrapping modes) should return the
     * base operator symbol so all variants sort together.
     *
     * @return Sort key string for ordering templates
     */
    [[nodiscard]]
    std::string sort_key() const;

    /**
     * Get the Mustache template string
     *
     * Returns the actual Mustache template that will be rendered with
     * the variables from prepare_variables().
     *
     * @return String view of the Mustache template
     */
    [[nodiscard]]
    std::string_view get_template() const;

    /**
     * Determine if this template applies to the given class
     *
     * Each template examines the ClassInfo to determine if it should
     * generate code for this strong type. For example, an addition operator
     * template would check if the class has the "+" operator enabled.
     *
     * @param info Strong type class information
     * @return true if this template should generate code
     */
    [[nodiscard]]
    bool should_apply(ClassInfo const & info) const;

    /**
     * Prepare variables for Mustache rendering
     *
     * Creates a JSON object containing all variables needed by the template.
     * This member function calls prepare_variables_impl() to get
     * template-specific variables, then adds common variables that all
     * templates may need:
     * - value: The member variable name (value or value_)
     * - const_expr: "constexpr " or empty based on settings
     *
     * Derived classes can override specific variables by setting them in
     * prepare_variables_impl(). The common variables are only set if not
     * already present in the variables object.
     *
     * @param info Strong type class information
     * @return JSON object with template variables
     */
    [[nodiscard]]
    boost::json::object prepare_variables(ClassInfo const & info) const;

    /**
     * Get required header includes for this template
     *
     * Returns the set of headers that must be included when this template
     * is applied. For example, hash templates require <functional>.
     *
     * @return Set of header file paths (e.g., "<functional>", "<utility>")
     */
    [[nodiscard]]
    std::set<std::string> required_includes() const;

    /**
     * Get required preamble code for this template
     *
     * Some templates require helper code in the preamble section
     * (e.g., type traits, helper functions). This member function returns
     * identifiers for preamble sections needed by this template.
     *
     * @return Set of preamble section identifiers
     */
    [[nodiscard]]
    std::set<std::string> required_preamble() const;

    /**
     * Validate that this template can be applied
     *
     * Performs validation beyond simple should_apply() checking.
     * Throws exceptions if the template cannot be correctly applied
     * due to conflicting options, missing required features, etc.
     *
     * @param info Strong type class information
     * @throws std::runtime_error if validation fails
     */
    void validate(ClassInfo const & info) const;

    /**
     * Render the template with the given class information
     *
     * This is the main entry point that combines all the other operations:
     * validates, prepares variables, and renders the Mustache template.
     *
     * @param info Strong type class information
     * @return Rendered C++ code
     * @throws std::runtime_error if rendering fails
     */
    [[nodiscard]]
    std::string render(ClassInfo const & info) const;

protected:
    // Protected virtual implementation member functions for NVI pattern

    virtual std::string id_impl() const = 0;
    virtual std::string_view get_template_impl() const = 0;
    virtual bool should_apply_impl(ClassInfo const &) const = 0;
    virtual boost::json::object prepare_variables_impl(ClassInfo const &) const;
    virtual std::string sort_key_impl() const;
    virtual std::set<std::string> required_includes_impl() const;
    virtual std::set<std::string> required_preamble_impl() const;
    virtual void validate_impl(ClassInfo const &) const;
    virtual std::string render_impl(ClassInfo const &) const;
};

}} // namespace wjh::atlas::generation::v1

#endif // WJH_ATLAS_7A3F9E2C8B4D5F6A1C9E7B3D8A5F2C4E
