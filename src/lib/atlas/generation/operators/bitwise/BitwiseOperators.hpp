// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_A439AF2F4C8044719C8B9A1196A0815B
#define WJH_ATLAS_A439AF2F4C8044719C8B9A1196A0815B

#include "atlas/generation/core/ITemplate.hpp"

namespace wjh::atlas::generation {

// Forward declarations
struct ClassInfo;

/**
 * Base class for bitwise operator templates
 *
 * Provides common template implementation for all bitwise operators.
 * All bitwise operators use the same template (operator+= forwarding pattern).
 */
class BitwiseOperatorBase
: public ITemplate
{
protected:
    [[nodiscard]]
    std::string_view get_template_impl() const noexcept override;

    [[nodiscard]]
    boost::json::object prepare_variables_for_operator(
        ClassInfo const & info,
        std::string_view op_symbol) const;
};

// Bitwise AND operator (&)
class BitwiseAndOperatorBase
: public BitwiseOperatorBase
{
protected:
    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;

    [[nodiscard]]
    boost::json::object prepare_variables_impl(
        ClassInfo const & info) const override;

    [[nodiscard]]
    std::string sort_key_impl() const override
    {
        return "&";
    }
};

class DefaultBitwiseAndOperator final
: public BitwiseAndOperatorBase
{
protected:
    [[nodiscard]]
    std::string id_impl() const override
    {
        return "operators.bitwise.and.default";
    }

    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;
};

// Bitwise OR operator (|)
class BitwiseOrOperatorBase
: public BitwiseOperatorBase
{
protected:
    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;

    [[nodiscard]]
    boost::json::object prepare_variables_impl(
        ClassInfo const & info) const override;

    [[nodiscard]]
    std::string sort_key_impl() const override
    {
        return "|";
    }
};

class DefaultBitwiseOrOperator final
: public BitwiseOrOperatorBase
{
protected:
    [[nodiscard]]
    std::string id_impl() const override
    {
        return "operators.bitwise.or.default";
    }

    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;
};

// Bitwise XOR operator (^)
class BitwiseXorOperatorBase
: public BitwiseOperatorBase
{
protected:
    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;

    [[nodiscard]]
    boost::json::object prepare_variables_impl(
        ClassInfo const & info) const override;

    [[nodiscard]]
    std::string sort_key_impl() const override
    {
        return "^";
    }
};

class DefaultBitwiseXorOperator final
: public BitwiseXorOperatorBase
{
protected:
    [[nodiscard]]
    std::string id_impl() const override
    {
        return "operators.bitwise.xor.default";
    }

    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;
};

// Left shift operator (<<)
class LeftShiftOperatorBase
: public BitwiseOperatorBase
{
protected:
    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;

    [[nodiscard]]
    boost::json::object prepare_variables_impl(
        ClassInfo const & info) const override;

    [[nodiscard]]
    std::string sort_key_impl() const override
    {
        return "<<";
    }
};

class DefaultLeftShiftOperator final
: public LeftShiftOperatorBase
{
protected:
    [[nodiscard]]
    std::string id_impl() const override
    {
        return "operators.bitwise.left_shift.default";
    }

    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;
};

// Right shift operator (>>)
class RightShiftOperatorBase
: public BitwiseOperatorBase
{
protected:
    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;

    [[nodiscard]]
    boost::json::object prepare_variables_impl(
        ClassInfo const & info) const override;

    [[nodiscard]]
    std::string sort_key_impl() const override
    {
        return ">>";
    }
};

class DefaultRightShiftOperator final
: public RightShiftOperatorBase
{
protected:
    [[nodiscard]]
    std::string id_impl() const override
    {
        return "operators.bitwise.right_shift.default";
    }

    [[nodiscard]]
    bool should_apply_impl(ClassInfo const & info) const override;
};

} // namespace wjh::atlas::generation

#endif // WJH_ATLAS_A439AF2F4C8044719C8B9A1196A0815B
