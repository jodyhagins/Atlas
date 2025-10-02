# CMake Integration

## FetchContent (The Easy Way)

Because who wants to manually install dependencies like it's 1999?

```cmake
include(FetchContent)
FetchContent_Declare(Atlas
    GIT_REPOSITORY https://github.com/jodyhagins/Atlas.git
    GIT_TAG main)
FetchContent_MakeAvailable(Atlas)

# Escape semicolons because CMake treats them like list separators
# (yes, this is annoying, no we can't fix it)
set(DESC "strong int\\; ==, !=, <=>")

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/UserId.hpp
    COMMAND ${Atlas_EXECUTABLE}
        --kind=struct --namespace=myapp --name=UserId
        --description=${DESC}
        --output=${CMAKE_CURRENT_BINARY_DIR}/UserId.hpp
    DEPENDS Atlas::atlas
    VERBATIM)
```

## Reusable Function (DRY Principle Applied)

Stop copy-pasting like a caveman:

```cmake
function(generate_strong_type NAMESPACE TYPE_NAME DESCRIPTION)
    set(HEADER_FILE "${CMAKE_CURRENT_BINARY_DIR}/${TYPE_NAME}.hpp")
    string(REPLACE ";" "\\;" DESC_ESCAPED "${DESCRIPTION}")

    add_custom_command(
        OUTPUT ${HEADER_FILE}
        COMMAND ${Atlas_EXECUTABLE}
            --kind=struct
            --namespace=${NAMESPACE}
            --name=${TYPE_NAME}
            --description=${DESC_ESCAPED}
            --output=${HEADER_FILE}
        DEPENDS Atlas::atlas
        VERBATIM)

    set(GENERATED_HEADERS ${GENERATED_HEADERS} ${HEADER_FILE} PARENT_SCOPE)
endfunction()

# Now generate to your heart's content
generate_strong_type("myapp" "UserId" "strong int; ==, !=")
generate_strong_type("myapp" "Price" "strong double; +, -, *")
```

## Using find_package (The Traditional Way)

For the control freaks who want to install things properly:

```bash
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
cmake --install build
```

Then in your project:
```cmake
find_package(Atlas REQUIRED)

add_custom_command(
    OUTPUT UserId.hpp
    COMMAND ${Atlas_EXECUTABLE} ...
    DEPENDS Atlas::atlas)
```

## Pro Tips

Speed up builds if you already have Boost installed:
```cmake
set(USE_SYSTEM_BOOST ON CACHE BOOL "Use system Boost")
FetchContent_MakeAvailable(Atlas)
```

## What You Get

| Variable/Target | Description |
|----------------|-------------|
| `Atlas::atlas` | The executable target (always use in DEPENDS) |
| `${Atlas_EXECUTABLE}` | Path to the binary (for COMMAND) |