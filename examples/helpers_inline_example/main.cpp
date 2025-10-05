// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------

#include "CommonTypes.hpp"

#include <chrono>

#include <iostream>

int
main()
{
    std::cout << "Atlas CMake Helpers - Inline Types Example\n";
    std::cout << "===========================================\n\n";

    // All types defined inline in CMakeLists.txt!
    app::UserId user1{100};
    app::UserId user2{200};
    app::UserId user3; // Uses default value of -1

    std::cout << "UserId examples:\n";
    std::cout << "  user1: " << user1.value << "\n";
    std::cout << "  user2: " << user2.value << "\n";
    std::cout << "  user3 (default): " << user3.value << "\n";
    std::cout << "  user1 == user2: " << (user1 == user2 ? "true" : "false")
        << "\n";
    std::cout << "\n";

    app::SessionId session1{42};
    app::SessionId session2{42};

    std::cout << "SessionId examples:\n";
    std::cout << "  session1 == session2: "
        << (session1 == session2 ? "true" : "false") << "\n";
    std::cout << "\n";

    app::Distance d1{100.5};
    app::Distance d2{50.25};

    std::cout << "Distance examples:\n";
    std::cout << "  d1: " << d1 << "\n";
    std::cout << "  d2: " << d2 << "\n";
    std::cout << "  d1 + d2: " << (d1 + d2) << "\n";
    std::cout << "  d1 - d2: " << (d1 - d2) << "\n";
    std::cout << "  d1 / d2: " << (d1 / d2) << "\n";
    std::cout << "\n";

    auto now = std::chrono::system_clock::now().time_since_epoch().count();
    app::Timestamp t1{now};
    app::Timestamp t2{now + 1000};

    std::cout << "Timestamp examples:\n";
    std::cout << "  t1: " << t1 << "\n";
    std::cout << "  t2: " << t2 << "\n";
    std::cout << "  t1 < t2: " << (t1 < t2 ? "true" : "false") << "\n";
    std::cout << "  t1 >= t2: " << (t1 >= t2 ? "true" : "false") << "\n";

    std::cout << "\nAll inline type examples completed successfully!\n";
    return 0;
}
