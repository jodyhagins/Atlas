#!/bin/bash
# Test script to verify Atlas installation and find_package works correctly
# This simulates what the CI "test-cmake-package" job does

set -e

echo "🧪 Testing Atlas installation and find_package..."
echo ""

# Clean up any previous test artifacts
rm -rf build-install-test install-test-prefix test-package

# Step 1: Build and install Atlas
echo "📦 Step 1: Building and installing Atlas..."
cmake -B build-install-test \
    -DCMAKE_BUILD_TYPE=Release \
    -DATLAS_BUILD_TESTS=OFF
cmake --build build-install-test
cmake --install build-install-test --prefix install-test-prefix

echo "✅ Atlas installed to install-test-prefix/"
echo ""

# Step 2: Create a test project that uses find_package(Atlas)
echo "📝 Step 2: Creating test project..."
mkdir -p test-package
cd test-package

cat > test.cpp << 'EOF'
#include <iostream>

int main() {
    std::cout << "✅ Atlas find_package test passed!\n";
    return 0;
}
EOF

cat > CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.20)
project(TestFindPackage CXX)
set(CMAKE_CXX_STANDARD 20)
find_package(Atlas REQUIRED)
add_executable(test_find test.cpp)

# Use Atlas executable to generate a strong type
add_custom_command(
    OUTPUT generated.hpp
    COMMAND ${Atlas_EXECUTABLE} --kind=struct --namespace=test --name=TestType "--description=strong int" > generated.hpp
    COMMENT "Generating test strong type"
    VERBATIM
)
add_custom_target(gen_types DEPENDS generated.hpp)
add_dependencies(test_find gen_types)
EOF

echo "✅ Test project created"
echo ""

# Step 3: Configure and build the test project
echo "🔨 Step 3: Building test project..."
cmake -B build \
    -DCMAKE_PREFIX_PATH=../install-test-prefix
cmake --build build

echo "✅ Test project built"
echo ""

# Step 4: Run the test
echo "🚀 Step 4: Running test..."
./build/test_find

echo ""

# Step 5: Verify generated header
echo "✅ Step 5: Verifying generated code..."
if [ -f build/generated.hpp ]; then
    echo "✅ Generated header exists"
    if grep -q "struct TestType" build/generated.hpp; then
        echo "✅ Generated header contains expected content"
    else
        echo "❌ Generated header missing expected content"
        exit 1
    fi
else
    echo "❌ Generated header not found"
    exit 1
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "✅ All installation tests passed!"
echo ""
echo "Cleaning up..."
cd ..
rm -rf build-install-test install-test-prefix test-package
echo "✅ Cleanup complete"
