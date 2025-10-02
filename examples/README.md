# Atlas Examples

Examples that demonstrate what this thing can actually do.

## Files

**`strong_types_showcase.txt`** - The greatest hits
- Multiple namespaces and base types (because one is never enough)
- All the operators: arithmetic, comparison, bitwise, stream
- Default values, hash support, subscripts
- Constexpr control for when compile-time evaluation betrays you

**`interactions_showcase.txt`** - When types need to work together
- Cross-type operators (e.g., `Distance / Time -> Velocity`)
- Template interactions with C++20 concepts and C++17 SFINAE fallbacks
- Symmetric operations (`Price * int <-> Price`)
- Custom value accessors for the truly adventurous

## Generate Examples

Easy mode:
```bash
./generate_showcase.sh
```

Manual mode (for the DIY crowd):
```bash
cd ../build/atlas/debug && ninja

./bin/atlas --input=../../src/atlas/examples/strong_types_showcase.txt \
    --output=../../src/atlas/examples/generated/strong_types_showcase.hpp

./bin/atlas --interactions=true \
    --input=../../src/atlas/examples/interactions_showcase.txt \
    --output=../../src/atlas/examples/generated/interactions_showcase.hpp
```

Output lands in: `generated/strong_types_showcase.hpp`, `generated/interactions_showcase.hpp`