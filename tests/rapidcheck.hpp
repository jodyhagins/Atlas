// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_C277B09170A04B1386A72F9034CBA4A2
#define WJH_ATLAS_C277B09170A04B1386A72F9034CBA4A2

// clang-format off
#include "doctest.hpp"
// clang-format on

#include <rapidcheck/doctest.h>

#include <sstream>

#include <rapidcheck.h>

namespace wjh::atlas::testing::rc {

struct CheckCfg
: ::rc::detail::TestParams
{
    std::string description;
    bool verbose = false;
};

namespace detail {

struct Check
{
    auto operator () (
        CheckCfg cfg,
        auto && testable,
        std::source_location s = std::source_location::current()) const
    {
        using namespace ::rc::detail;
        namespace doctest = ::doctest;
        using namespace doctest::detail;

        if (cfg.seed == 0) {
            cfg.seed = doctest::getContextOptions()->rand_seed;
        }
        DOCTEST_SUBCASE(cfg.description.c_str())
        {
            TestMetadata metadata;
            metadata.id = cfg.description;
            metadata.description = cfg.description;

            if (auto r = checkTestable(
                    std::forward<decltype(testable)>(testable),
                    metadata,
                    cfg);
                r.template is<SuccessResult>())
            {
                if (not r.template get<SuccessResult>().distribution.empty() ||
                    cfg.verbose)
                {
                    std::cout << "- " << cfg.description << std::endl;
                    printResultMessage(r, std::cout);
                    std::cout << std::endl;
                }
#if defined(DOCTEST_CONFIG_ASSERTS_RETURN_VALUES)
                return
#endif
                    REQUIRE(true);
            } else {
                std::ostringstream o;
                printResultMessage(r, o << '\n');
                DOCTEST_INFO(o.str());
                ResultBuilder b(
                    doctest::assertType::DT_CHECK,
                    s.file_name(),
                    static_cast<int>(s.line()),
                    s.function_name());
                DOCTEST_ASSERT_LOG_REACT_RETURN(b);
            }
        }
#if defined(DOCTEST_CONFIG_ASSERTS_RETURN_VALUES)
        return REQUIRE(false);
#endif
    }

    auto operator () (
        auto && testable,
        std::source_location s = std::source_location::current()) const
    {
        return (
            *this)(CheckCfg{}, std::forward<decltype(testable)>(testable), s);
    }

    auto operator () (
        std::string_view description,
        auto && testable,
        std::source_location s = std::source_location::current()) const
    {
        return (*this)(
            [&] {
                auto cfg = CheckCfg{};
                cfg.description = std::string(description);
                return cfg;
            }(),
            std::forward<decltype(testable)>(testable),
            s);
    }
};

} // namespace detail

inline constexpr auto check = detail::Check{};

using namespace ::rc;

} // namespace wjh::atlas::testing::rc

#endif // WJH_ATLAS_C277B09170A04B1386A72F9034CBA4A2
