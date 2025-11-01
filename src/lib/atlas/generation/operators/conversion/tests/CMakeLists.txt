# ----------------------------------------------------------------------
# Copyright 2025 Jody Hagins
# Distributed under the MIT Software License
# See accompanying file LICENSE or copy at
# https://opensource.org/licenses/MIT
# ----------------------------------------------------------------------

add_executable(conversion_operators_ut
    ConversionOperators_ut.cpp)

target_link_libraries(conversion_operators_ut
    PRIVATE
        atlas_lib
        doctest::doctest)

target_apply_atlas_warnings(conversion_operators_ut)

add_test(NAME ConversionOperators COMMAND conversion_operators_ut)
