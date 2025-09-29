# FetchContent Example

This example demonstrates how to integrate Atlas into a CMake project using `FetchContent`.

## What This Example Shows

- How to fetch Atlas as a dependency using `FetchContent_Declare` and `FetchContent_MakeAvailable`
- How to use the `${Atlas_EXECUTABLE}` variable in custom commands
- How to generate multiple strong type headers during the build
- How to properly set up dependencies with `DEPENDS Atlas::atlas`
- How to include generated headers in your application

## Building and Running

From this directory:

```bash
cmake -B build
cmake --build build
./build/example_app
```

Expected output:
```
User IDs are different
Total price: 24.99
Price after discount: 14.99
Double price: 39.98
price1 is greater than price2
FetchContent integration test passed!
```

## Key Points

1. **Atlas is fetched automatically** - No need to install it separately
2. **Generated headers go to build directory** - Keeps source tree clean
3. **Custom commands depend on Atlas::atlas** - Ensures proper build ordering
4. **Include directories are configured** - Generated headers are found by the compiler

## Adapting for Your Project

To use this in your own project:

1. Change `SOURCE_DIR` to a `GIT_REPOSITORY` URL:
   ```cmake
   FetchContent_Declare(
       Atlas
       GIT_REPOSITORY https://github.com/jodyhagins/Atlas.git
       GIT_TAG main
   )
   ```

2. Adjust the type descriptions to match your domain types

3. Update the namespace from `example` to your project's namespace