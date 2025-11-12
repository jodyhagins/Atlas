// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#ifndef WJH_ATLAS_B306007996634FDDB32F24301C6C15D9
#define WJH_ATLAS_B306007996634FDDB32F24301C6C15D9

#include "StrongTypeGenerator.hpp"

#include <boost/describe.hpp>
#include <boost/json/value_from.hpp>

// BOOST_DESCRIBE metadata for StrongTypeDescription
// This must be in a header so that boost::json::value_from can use it
// in any translation unit that needs to serialize StrongTypeDescription
BOOST_DESCRIBE_STRUCT(
    wjh::atlas::StrongTypeDescription,
    (),
    (kind,
     type_namespace,
     type_name,
     description,
     default_value,
     guard_prefix,
     guard_separator,
     upcase_guard,
     generate_iterators,
     generate_formatter,
     cpp_standard))

#endif // WJH_ATLAS_B306007996634FDDB32F24301C6C15D9
