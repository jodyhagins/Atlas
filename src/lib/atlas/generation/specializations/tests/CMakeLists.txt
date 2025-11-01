# ----------------------------------------------------------------------
# Copyright 2025 Jody Hagins
# Distributed under the MIT Software License
# See accompanying file LICENSE or copy at
# https://opensource.org/licenses/MIT
# ----------------------------------------------------------------------

add_executable(specializations_ut
    Specializations_ut.cpp)

target_link_libraries(specializations_ut
    PRIVATE
        atlas_lib
        doctest::doctest)

target_apply_atlas_warnings(specializations_ut)

add_test(NAME Specializations COMMAND specializations_ut)
