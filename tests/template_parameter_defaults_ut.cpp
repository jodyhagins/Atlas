// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "error_test_support.hpp"

#include <string>

#include "doctest.hpp"

using namespace wjh::atlas::testing;

namespace {

// Convenience: assert atlas succeeded and return generated stdout.
//
// All success-path tests need a global namespace; the parser otherwise
// emits "Incomplete type definition" because templates inherit no
// fallback namespace. Tests can opt out by passing already-namespaced
// content; we only prepend if the caller didn't.
std::string
generate_or_fail(std::string const & content)
{
    std::string augmented = content.find("namespace=") == std::string::npos
        ? "namespace=test\n" + content
        : content;
    auto result = run_atlas_on_content(augmented);
    INFO("exit_code = " << result.exit_code);
    INFO("stdout = " << result.stdout_output);
    INFO("stderr = " << result.stderr_output);
    REQUIRE_FALSE(result.had_error());
    return result.stdout_output;
}

TEST_SUITE("Template Parameter Defaults: Header Parsing")
{
    TEST_CASE("Single parameter with default - no spaces around equals")
    {
        // [template Bool T=bool] - canonical form
        auto out = generate_or_fail(R"(
[template Bool T=bool]
description=strong {T}; ==, bool

[use Bool as IsAdmin]
)");
        // The generated type should wrap bool.
        CHECK(out.find("struct IsAdmin") != std::string::npos);
        CHECK(out.find("bool value") != std::string::npos);
    }

    TEST_CASE("Single parameter with default - space before equals")
    {
        auto out = generate_or_fail(R"(
[template Bool T =bool]
description=strong {T}; ==, bool

[use Bool as IsAdmin]
)");
        CHECK(out.find("struct IsAdmin") != std::string::npos);
        CHECK(out.find("bool value") != std::string::npos);
    }

    TEST_CASE("Single parameter with default - space after equals")
    {
        auto out = generate_or_fail(R"(
[template Bool T= bool]
description=strong {T}; ==, bool

[use Bool as IsAdmin]
)");
        CHECK(out.find("struct IsAdmin") != std::string::npos);
        CHECK(out.find("bool value") != std::string::npos);
    }

    TEST_CASE("Single parameter with default - spaces on both sides")
    {
        auto out = generate_or_fail(R"(
[template Bool T = bool]
description=strong {T}; ==, bool

[use Bool as IsAdmin]
)");
        CHECK(out.find("struct IsAdmin") != std::string::npos);
        CHECK(out.find("bool value") != std::string::npos);
    }

    TEST_CASE("Multiple parameters - both have defaults")
    {
        auto out = generate_or_fail(R"(
[template Pair K=int V=double]
description=strong std::pair<{K}, {V}>; ==

[use Pair as DefaultPair]
)");
        CHECK(out.find("struct DefaultPair") != std::string::npos);
        CHECK(
            out.find("std::pair<int, double> value") != std::string::npos);
    }

    TEST_CASE("Mixed required and defaulted parameters - default trails")
    {
        auto out = generate_or_fail(R"(
[template Pair K V=double]
description=strong std::pair<{K}, {V}>; ==

[use Pair int as IntDoublePair]
)");
        CHECK(out.find("struct IntDoublePair") != std::string::npos);
        CHECK(
            out.find("std::pair<int, double> value") != std::string::npos);
    }

    TEST_CASE("Default with qualified type name")
    {
        auto out = generate_or_fail(R"(
[template Vec T=std::int32_t]
description=strong std::vector<{T}>; ==

[use Vec as DefaultVec]
)");
        CHECK(out.find("struct DefaultVec") != std::string::npos);
        CHECK(
            out.find("std::vector<std::int32_t> value") != std::string::npos);
    }
}

TEST_SUITE("Template Parameter Defaults: Registration Errors")
{
    TEST_CASE("Required parameter cannot follow defaulted parameter")
    {
        auto result = test_input_content_error(R"(
[template Bad T=int U]
description=strong std::pair<{T}, {U}>; ==
)");
        CHECK(result.had_error());
        auto err = result.stderr_output + result.stdout_output;
        // Message should explain the ordering rule.
        CHECK(
            (err.find("default") != std::string::npos ||
             err.find("Default") != std::string::npos));
    }

    TEST_CASE("Required after default catches second of three params")
    {
        auto result = test_input_content_error(R"(
[template Bad K V=double L]
description=strong tuple<{K},{V},{L}>; ==
)");
        CHECK(result.had_error());
    }
}

TEST_SUITE("Template Parameter Defaults: Header Syntax Errors")
{
    TEST_CASE("Equals at start of parameter list with no name")
    {
        auto result = test_input_content_error(R"(
[template Bad =bool]
description=strong {bool}; ==
)");
        CHECK(result.had_error());
        auto err = result.stderr_output + result.stdout_output;
        CHECK(
            (err.find("=") != std::string::npos ||
             err.find("Invalid") != std::string::npos));
    }

    TEST_CASE("Bare equals between params")
    {
        auto result = test_input_content_error(R"(
[template Bad T = = bool]
description=strong {T}; ==
)");
        CHECK(result.had_error());
    }

    TEST_CASE("Trailing equals with no value (T=)")
    {
        auto result = test_input_content_error(R"(
[template Bad T=]
description=strong {T}; ==
)");
        CHECK(result.had_error());
        auto err = result.stderr_output + result.stdout_output;
        CHECK(
            (err.find("default value") != std::string::npos ||
             err.find("missing") != std::string::npos));
    }

    TEST_CASE("Double equals (T==bool)")
    {
        auto result = test_input_content_error(R"(
[template Bad T==bool]
description=strong {T}; ==
)");
        CHECK(result.had_error());
    }

    TEST_CASE("Equals sign in default value (T=a=b)")
    {
        auto result = test_input_content_error(R"(
[template Bad T=foo=bar]
description=strong {T}; ==
)");
        CHECK(result.had_error());
    }

    TEST_CASE("Invalid identifier as parameter name with default")
    {
        auto result = test_input_content_error(R"(
[template Bad 123T=bool]
description=strong {123T}; ==
)");
        CHECK(result.had_error());
        auto err = result.stderr_output + result.stdout_output;
        CHECK(err.find("Invalid") != std::string::npos);
    }
}

TEST_SUITE("Template Parameter Defaults: Instantiation")
{
    TEST_CASE("Instantiation omitting all args uses defaults")
    {
        auto out = generate_or_fail(R"(
[template Bool T=bool]
description=strong {T}; ==, bool

[use Bool as IsAdmin]
)");
        CHECK(out.find("struct IsAdmin") != std::string::npos);
        CHECK(out.find("bool value") != std::string::npos);
    }

    TEST_CASE("Instantiation overriding default still works")
    {
        auto out = generate_or_fail(R"(
[template Bool T=bool]
description=strong {T}; ==

[use Bool int as IntFlag]
)");
        CHECK(out.find("struct IntFlag") != std::string::npos);
        CHECK(out.find("int value") != std::string::npos);
    }

    TEST_CASE("Auto-generated name uses defaults when no args given")
    {
        // No 'as' clause; the auto-generated name should incorporate the
        // defaulted argument(s) so [use Bool] and [use Bool bool] collide.
        auto out = generate_or_fail(R"(
[template Bool T=bool]
description=strong {T}; ==, bool

[use Bool]
)");
        // Expect the generated name to be Bool_bool, not just Bool.
        CHECK(out.find("struct Bool_bool") != std::string::npos);
    }

    TEST_CASE("Default value propagates into generated default_value")
    {
        auto out = generate_or_fail(R"(
[template Flag T=bool]
description=strong {T}; ==, bool
default_value=false

[use Flag as IsActive]
)");
        // The wrapped member should be initialized with the template's
        // default_value, with {T} substituted to bool.
        CHECK(
            out.find("bool value = static_cast<bool>(false)") !=
            std::string::npos);
    }

    TEST_CASE("Mixed required/defaulted instantiation - omit only the default")
    {
        auto out = generate_or_fail(R"(
[template Pair K V=double]
description=strong std::pair<{K}, {V}>; ==

[use Pair int as P]
)");
        CHECK(
            out.find("std::pair<int, double> value") != std::string::npos);
    }

    TEST_CASE("Mixed required/defaulted instantiation - override the default")
    {
        auto out = generate_or_fail(R"(
[template Pair K V=double]
description=strong std::pair<{K}, {V}>; ==

[use Pair int float as P]
)");
        CHECK(
            out.find("std::pair<int, float> value") != std::string::npos);
    }
}

TEST_SUITE("Template Parameter Defaults: Instantiation Errors")
{
    TEST_CASE("Missing required arg with no default still fails")
    {
        auto result = test_input_content_error(R"(
[template Pair K V=double]
description=strong std::pair<{K}, {V}>; ==

[use Pair as Bad]
)");
        CHECK(result.had_error());
        auto err = result.stderr_output + result.stdout_output;
        CHECK(
            (err.find("argument") != std::string::npos ||
             err.find("default") != std::string::npos));
    }

    TEST_CASE("Too many arguments still fails when defaults present")
    {
        auto result = test_input_content_error(R"(
[template Bool T=bool]
description=strong {T}; ==

[use Bool int double as Bad]
)");
        CHECK(result.had_error());
        auto err = result.stderr_output + result.stdout_output;
        CHECK(
            (err.find("argument") != std::string::npos ||
             err.find("expects") != std::string::npos));
    }
}

TEST_SUITE("Template Parameter Defaults: Composition")
{
    TEST_CASE("Default works alongside profile expansion")
    {
        auto out = generate_or_fail(R"(
profile=NUM; +, -, *, /

[template Number T=int]
description=strong {T}; {NUM}, ==

[use Number as DefaultNumber]
)");
        // Should generate arithmetic operators on int.
        CHECK(out.find("struct DefaultNumber") != std::string::npos);
        CHECK(out.find("int value") != std::string::npos);
    }

    TEST_CASE("Default works alongside instance overrides")
    {
        auto out = generate_or_fail(R"(
[template Bool T=bool]
description=strong {T}; ==, bool

[use Bool as ClassBool]
kind=class
)");
        CHECK(out.find("class ClassBool") != std::string::npos);
    }

    TEST_CASE("Default works alongside template-level default_value")
    {
        auto out = generate_or_fail(R"(
[template Bool T=bool]
description=strong {T}; ==
default_value=true

[use Bool as Flag]
)");
        CHECK(
            out.find("bool value = static_cast<bool>(true)") !=
            std::string::npos);
    }

    TEST_CASE("Instance default_value overrides template default_value")
    {
        auto out = generate_or_fail(R"(
[template Bool T=bool]
description=strong {T}; ==
default_value=true

[use Bool as Flag]
default_value=false
)");
        CHECK(
            out.find("bool value = static_cast<bool>(false)") !=
            std::string::npos);
    }

    TEST_CASE("Multiple [use] of same default-bearing template")
    {
        auto out = generate_or_fail(R"(
[template Bool T=bool]
description=strong {T}; ==, bool

[use Bool as IsAdmin]
[use Bool as IsActive]
[use Bool as IsVerified]
)");
        CHECK(out.find("struct IsAdmin") != std::string::npos);
        CHECK(out.find("struct IsActive") != std::string::npos);
        CHECK(out.find("struct IsVerified") != std::string::npos);
    }

    TEST_CASE("Per-instance constants on default-bearing template")
    {
        // The realistic use case: one [template Bool T=bool] with each
        // [use] supplying its own domain-specific constants.
        auto out = generate_or_fail(R"(
[template Bool T=bool]
description=strong {T}; ==, bool
default_value=false

[use Bool as IsAdmin]
constants=admin:true; non_admin:false

[use Bool as IsActive]
constants=enabled:true; disabled:false

[use Bool as IsVerified]
constants=verified:true; unverified:false; pending:false
)");
        // Each instance has only its own constants.
        CHECK(out.find("static const IsAdmin admin") != std::string::npos);
        CHECK(out.find("static const IsAdmin non_admin") != std::string::npos);
        CHECK(out.find("static const IsActive enabled") != std::string::npos);
        CHECK(
            out.find("static const IsActive disabled") != std::string::npos);
        CHECK(
            out.find("static const IsVerified verified") != std::string::npos);
        CHECK(
            out.find("static const IsVerified unverified") !=
            std::string::npos);
        CHECK(
            out.find("static const IsVerified pending") != std::string::npos);

        // Constants must NOT bleed across instances.
        CHECK(
            out.find("static const IsAdmin enabled") == std::string::npos);
        CHECK(
            out.find("static const IsActive admin") == std::string::npos);
        CHECK(
            out.find("static const IsAdmin verified") == std::string::npos);
    }

    TEST_CASE("Template constants merge with per-instance constants")
    {
        // Template provides common values; instances extend with domain
        // names. Both should appear on each generated type.
        auto out = generate_or_fail(R"(
[template Bool T=bool]
description=strong {T}; ==, bool
default_value=false
constants=true_value:true; false_value:false

[use Bool as IsAdmin]
constants=admin:true; non_admin:false

[use Bool as IsActive]
)");
        // IsAdmin gets template constants AND its own.
        CHECK(
            out.find("static const IsAdmin true_value") != std::string::npos);
        CHECK(
            out.find("static const IsAdmin false_value") != std::string::npos);
        CHECK(out.find("static const IsAdmin admin") != std::string::npos);
        CHECK(out.find("static const IsAdmin non_admin") != std::string::npos);

        // IsActive only gets the template constants (no instance constants).
        CHECK(
            out.find("static const IsActive true_value") != std::string::npos);
        CHECK(
            out.find("static const IsActive false_value") !=
            std::string::npos);
        CHECK(out.find("static const IsActive admin") == std::string::npos);
    }
}

TEST_SUITE("Template Parameter Defaults: Backwards Compatibility")
{
    TEST_CASE("Templates without defaults still work the same")
    {
        // Pre-existing behavior: required parameter, must be supplied.
        auto out = generate_or_fail(R"(
[template Optional T]
description=strong std::optional<{T}>; <=>, bool

[use Optional int as OptInt]
)");
        CHECK(out.find("struct OptInt") != std::string::npos);
        CHECK(out.find("std::optional<int> value") != std::string::npos);
    }

    TEST_CASE("Templates without defaults still reject missing args")
    {
        auto result = test_input_content_error(R"(
[template Optional T]
description=strong std::optional<{T}>; <=>, bool

[use Optional as Bad]
)");
        CHECK(result.had_error());
    }

    TEST_CASE("Existing duplicate parameter check still fires")
    {
        auto result = test_input_content_error(R"(
[template Bad T=int T=bool]
description=strong {T}; ==
)");
        CHECK(result.had_error());
        auto err = result.stderr_output + result.stdout_output;
        CHECK(err.find("Duplicate") != std::string::npos);
    }
}

} // anonymous namespace
