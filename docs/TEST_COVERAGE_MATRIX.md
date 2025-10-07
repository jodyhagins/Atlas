# Atlas Test Coverage Matrix

**Generated**: Phase 3 Completion
**Total Golden Tests**: 68
**Test Execution Time**: < 150ms

---

## Overview

This document maps Atlas features to their test coverage across different test types:
- **Golden File Tests**: Exact output verification (regression testing)
- **Property Tests**: Semantic correctness (algebraic properties) - *Phase 4*
- **Compilation Tests**: Type safety (what should fail) - *Phase 5*

---

## Feature Coverage Summary

| Feature Category | Golden Tests | Property Tests | Compilation Tests |
|-----------------|--------------|----------------|-------------------|
| Arithmetic Operators | ✅ 8 tests | ⏳ Phase 4 | ⏳ Phase 5 |
| Comparison Operators | ✅ 5 tests | ⏳ Phase 4 | ⏳ Phase 5 |
| Bitwise Operators | ✅ 4 tests | N/A | ⏳ Phase 5 |
| Logical Operators | ✅ 3 tests | ⏳ Phase 4 | ⏳ Phase 5 |
| Unary Operators | ✅ 4 tests | ⏳ Phase 4 | ⏳ Phase 5 |
| Access Operators | ✅ 5 tests | N/A | ⏳ Phase 5 |
| Cast Operators | ✅ 7 tests | N/A | ⏳ Phase 5 |
| Hash Support | ✅ 4 tests | ⏳ Phase 4 | ⏳ Phase 5 |
| Formatter Support | ✅ 4 tests | N/A | ⏳ Phase 5 |
| Iterable Support | ✅ 3 tests | N/A | ⏳ Phase 5 |
| Stream Operators | ✅ 4 tests | N/A | ⏳ Phase 5 |
| Assignment Operator | ✅ 2 tests | N/A | N/A |
| Edge Cases | ✅ 10 tests | ⏳ Phase 4 | ⏳ Phase 5 |
| Integration | ✅ 10 tests | ⏳ Phase 4 | ⏳ Phase 5 |

**Total Coverage**: 68 golden file tests ✅

---

## Detailed Feature Mapping

### Arithmetic Operators (8 tests)

| Feature | Test File | Status | Notes |
|---------|-----------|--------|-------|
| `+` (addition) | `operators/arithmetic-add-subtract.input` | ✅ | Basic binary operator |
| `-` (subtraction) | `operators/arithmetic-add-subtract.input` | ✅ | Basic binary operator |
| `*` (multiplication) | `operators/arithmetic-multiply-divide.input` | ✅ | Basic binary operator |
| `/` (division) | `operators/arithmetic-multiply-divide.input` | ✅ | Basic binary operator |
| `%` (modulo) | `operators/arithmetic-modulo.input` | ✅ | Modulo operator |
| Combined arithmetic | `operators/arithmetic-basic.input` | ✅ | +, -, *, / together |
| All arithmetic | `operators/arithmetic-all.input` | ✅ | All arithmetic ops |
| Compound assignment | `operators/arithmetic-compound.input` | ✅ | +=, -=, *=, /=, %= |

**Property Tests (Phase 4)**: Commutativity, associativity, identity, inverse

---

### Comparison Operators (5 tests)

| Feature | Test File | Status | Notes |
|---------|-----------|--------|-------|
| `==`, `!=` (equality) | `operators/comparison-equality.input` | ✅ | Equality comparison |
| `<`, `<=`, `>`, `>=` (relational) | `operators/comparison-relational.input` | ✅ | Relational comparison |
| `<=>` (spaceship) | `operators/comparison-spaceship.input` | ✅ | C++20 three-way comparison |
| All comparisons | `operators/comparison-all.input` | ✅ | All comparison ops |
| Spaceship with std::set | `integration/spaceship-set.input` | ✅ | Integration test |

**Property Tests (Phase 4)**: Reflexivity, symmetry, transitivity, spaceship consistency

**Known Design Issue**: `<=>` makes `==`, `!=`, `<`, `<=`, `>`, `>=` redundant (warning emitted)

---

### Bitwise Operators (4 tests)

| Feature | Test File | Status | Notes |
|---------|-----------|--------|-------|
| `&`, `|`, `^` (binary) | `operators/bitwise-basic.input` | ✅ | Basic bitwise ops |
| `<<`, `>>` (shift) | `operators/bitwise-shift.input` | ✅ | Shift operators |
| `~` (complement) | `operators/bitwise-not.input` | ✅ | Unary bitwise NOT |
| All bitwise | `operators/bitwise-all.input` | ✅ | All bitwise ops |

**Known Design Decision**: Shift operators are symmetric (both operands strong type, not RHS=int)

---

### Logical Operators (3 tests)

| Feature | Test File | Status | Notes |
|---------|-----------|--------|-------|
| `&&`, `||` (logical and/or) | `operators/logical-and-or.input` | ✅ | Short-circuit operators |
| `!` (logical not) | `operators/logical-not.input` | ✅ | Unary logical NOT |
| All logical | `operators/logical-all.input` | ✅ | All logical ops |

---

### Unary Operators (4 tests)

| Feature | Test File | Status | Notes |
|---------|-----------|--------|-------|
| `++`, `--` (increment/decrement) | `operators/increment-decrement.input` | ✅ | Pre/post increment/decrement |
| `++` only | `operators/increment-only.input` | ✅ | Just increment |
| `+`, `-` (unary plus/minus) | `operators/unary-plus-minus.input` | ✅ | Unary arithmetic |
| All unary | `operators/unary-all.input` | ✅ | All unary operators |

---

### Access Operators (5 tests)

| Feature | Test File | Status | Notes |
|---------|-----------|--------|-------|
| `@` (dereference) | `operators/access-dereference.input` | ✅ | Dereference operator |
| `->` (arrow) | `operators/access-arrow.input` | ✅ | Member access |
| `[]` (subscript) | `operators/access-subscript.input` | ✅ | Subscript operator |
| `()` (call - nullary) | `operators/access-call-nullary.input` | ✅ | Function call operator |
| `(&)` (call - template) | `operators/access-call-template.input` | ✅ | Template function call |
| `&` (address-of) | `operators/access-address-of.input` | ✅ | Address-of operator |

---

### Cast Operators (7 tests)

| Feature | Test File | Status | Notes |
|---------|-----------|--------|-------|
| `cast<bool>` | `cast-operators/explicit-single-bool.input` | ✅ | Explicit cast to bool |
| `cast<int>` | `cast-operators/explicit-single-int.input` | ✅ | Explicit cast to int |
| Multiple explicit casts | `cast-operators/explicit-multiple.input` | ✅ | Multiple cast targets |
| `implicit_cast<bool>` | `cast-operators/implicit-bool.input` | ✅ | Implicit bool conversion |
| `implicit_cast<int>` | `cast-operators/implicit-int.input` | ✅ | Implicit int conversion |
| Mixed casts | `cast-operators/mixed-explicit-implicit.input` | ✅ | Both explicit and implicit |
| `cast<Type const&>` | `cast-operators/const-reference.input` | ✅ | Const reference cast |

**Compilation Tests (Phase 5)**: Cannot implicit cast when explicit required

---

### Hash Support (4 tests)

| Feature | Test File | Status | Notes |
|---------|-----------|--------|-------|
| `hash` with int | `special-features/hash-int.input` | ✅ | std::hash for int |
| `hash` with string | `special-features/hash-string.input` | ✅ | std::hash for std::string |
| `no-constexpr-hash` | `special-features/hash-no-constexpr.input` | ✅ | Hash without constexpr |
| Hash with unordered_map | `integration/hash-unordered-map.input` | ✅ | Integration test |

**Property Tests (Phase 4)**: Determinism, equality consistency

---

### Formatter Support (4 tests)

| Feature | Test File | Status | Notes |
|---------|-----------|--------|-------|
| `fmt` with int | `special-features/formatter-int.input` | ✅ | std::formatter for int |
| `fmt` with string | `special-features/formatter-string.input` | ✅ | std::formatter for string |
| `fmt` + other features | `special-features/formatter-combined.input` | ✅ | Formatter with operators |
| Formatter with std::format | `integration/formatter-format.input` | ✅ | Integration test |

---

### Iterable Support (3 tests)

| Feature | Test File | Status | Notes |
|---------|-----------|--------|-------|
| `iterable` with vector | `special-features/iterable-vector.input` | ✅ | Iterable std::vector |
| `iterable` with string | `special-features/iterable-string.input` | ✅ | Iterable std::string |
| Range-based for loop | `integration/iterable-range-for.input` | ✅ | Integration test |

---

### Stream Operators (4 tests)

| Feature | Test File | Status | Notes |
|---------|-----------|--------|-------|
| `out` (operator<<) | `special-features/stream-out.input` | ✅ | Stream insertion |
| `in` (operator>>) | `special-features/stream-in.input` | ✅ | Stream extraction |
| `in`, `out` together | `special-features/stream-both.input` | ✅ | Both stream operators |
| Stream with stringstream | `integration/stream-stringstream.input` | ✅ | Integration test |

**Known Design Issue**: Stream operators can conflict with shift operators

---

### Assignment Operator (2 tests)

| Feature | Test File | Status | Notes |
|---------|-----------|--------|-------|
| `assign` with int | `special-features/assign-int.input` | ✅ | Assignment from int |
| `assign` with string | `special-features/assign-string.input` | ✅ | Assignment from string |

---

### Edge Cases (10 tests)

| Feature | Test File | Status | Notes |
|---------|-----------|--------|-------|
| Move-only types (unique_ptr) | `edge-cases/move-only-unique-ptr.input` | ✅ | Move semantics |
| Empty types | `edge-cases/empty-type.input` | ✅ | Zero-size struct |
| Large types (array<1000>) | `edge-cases/large-type.input` | ✅ | Pass-by-reference |
| Const operations | `edge-cases/const-operations.input` | ✅ | Const-correctness |
| Explicit default ctor | `edge-cases/explicit-default-ctor.input` | ✅ | Design decision |
| Zero-overhead guarantee | `edge-cases/size-alignment.input` | ✅ | sizeof/alignof |
| Constexpr operations | `edge-cases/constexpr-operations.input` | ✅ | Compile-time evaluation |
| Noexcept verification | `edge-cases/noexcept-verification.input` | ✅ | Exception specifications |
| Complex template types | `edge-cases/complex-template-type.input` | ✅ | Nested templates |
| No-constexpr mode | `edge-cases/no-constexpr-all.input` | ✅ | Disable constexpr |

**Property Tests (Phase 4)**: Move semantics properties

---

### Integration Tests (10 tests)

| Feature | Test File | Status | Notes |
|---------|-----------|--------|-------|
| Hash + unordered_map | `integration/hash-unordered-map.input` | ✅ | Hash as map key |
| Formatter + std::format | `integration/formatter-format.input` | ✅ | C++20 formatting |
| Comparison + std::sort | `integration/comparison-sort.input` | ✅ | Sorting algorithms |
| Iterable + range-for | `integration/iterable-range-for.input` | ✅ | Range-based loops |
| Spaceship + std::set | `integration/spaceship-set.input` | ✅ | Ordered containers |
| Stream + stringstream | `integration/stream-stringstream.input` | ✅ | Stream I/O |
| Combined features | `integration/combined-features.input` | ✅ | Real-world usage |
| ODR violation - Type 1 | `integration/odr-violation-type1.input` | ✅ | Multi-file test 1/3 |
| ODR violation - Type 2 | `integration/odr-violation-type2.input` | ✅ | Multi-file test 2/3 |
| ODR violation - Type 3 | `integration/odr-violation-type3.input` | ✅ | Multi-file test 3/3 |

---

## Coverage Gaps & Action Items

### Phase 4 (Property Tests)

**Required property tests** (from Phase 2 review):

1. **Arithmetic Properties**
   - Commutativity: `x + y == y + x`
   - Associativity: `(x + y) + z == x + (y + z)`
   - Identity: `x + StrongInt(0) == x`
   - Inverse: `x - x == StrongInt(0)`

2. **Comparison Properties**
   - Reflexivity: `x == x`
   - Symmetry: `(x == y) implies (y == x)`
   - Transitivity: `(x < y && y < z) implies (x < z)`
   - Spaceship consistency: `(x <=> y) < 0 iff (x < y)`

3. **Hash Properties**
   - Deterministic: `hash(x) == hash(x)`
   - Equal objects have equal hashes: `(x == y) implies (hash(x) == hash(y))`

4. **Move Semantics Properties**
   - Move leaves object in valid state
   - Move is noexcept for trivially movable types

### Phase 5 (Compilation Tests)

**Required compilation failure tests**:

1. **Type Safety**
   - Cannot mix different strong types: `StrongMeters + StrongSeconds` ❌
   - Cannot mix strong type with underlying: `StrongInt + int` ❌
   - Cannot implicit cast when explicit required ❌
   - Cannot default-construct in aggregate context ❌

2. **SFINAE/Concepts**
   - Hash fails gracefully on non-hashable types
   - Formatter fails gracefully on non-formattable types

---

## Known Design Decisions

These are **intentional design choices**, not bugs:

1. **Shift operators are symmetric**
   - Both operands are strong type (not RHS=int like typical C++)
   - Documented in Phase 2 review findings

2. **Default constructor is explicit**
   - Prevents `std::vector<StrongInt>(10)` and aggregate initialization
   - Zero-overhead design choice

3. **Spaceship doesn't auto-generate operator==**
   - Only generates `<=>`
   - C++20 best practice: default both `<=>` and `==`
   - Generator emits warning when redundant operators specified

4. **Stream operators can conflict with shift operators**
   - Both use `<<` and `>>`
   - User must choose between stream I/O and bitwise shifts

---

## Performance Metrics

| Metric | Phase 2 | Phase 3 | Target |
|--------|---------|---------|--------|
| Golden file tests | 48 | 68 | 80-100 |
| Test execution time | 76ms | ~150ms | < 1000ms |
| Test code lines | ~500 | ~700 | ~2000 |

**Note**: Test count increased 42% with minimal performance impact.

---

## Test Organization

```
tests/fixtures/golden/
├── strong-types/
│   ├── operators/ (28 tests)
│   │   ├── arithmetic (8 tests)
│   │   ├── comparison (5 tests)
│   │   ├── bitwise (4 tests)
│   │   ├── logical (3 tests)
│   │   ├── unary (4 tests)
│   │   └── access (5 tests)
│   ├── cast-operators/ (7 tests)
│   ├── special-features/ (13 tests)
│   ├── edge-cases/ (10 tests - NEW in Phase 3)
│   └── header-generation/ (0 tests - Phase 3 future work)
└── integration/ (10 tests - NEW in Phase 3)
    ├── Standard library integration (7 tests)
    └── ODR violation multi-file (3 tests)
```

---

## Next Steps

1. **Phase 4**: Implement property-based tests
   - Create custom RapidCheck generators
   - Implement 5-10 property tests
   - Validate properties catch bugs

2. **Phase 5**: Implement compilation tests
   - Create 15+ focused compilation tests
   - Test type safety (what should fail)
   - Test SFINAE/concept graceful failures

3. **Phase 6**: Migration and cleanup
   - Coverage validation (ensure >= old tests)
   - Delete old fragmented tests
   - Update CI configuration
   - Update documentation

---

## Success Criteria

### Quantitative ✅
- ✅ 68/80-100 golden file test pairs (85% complete)
- ⏳ 0/5-10 property tests (Phase 4)
- ⏳ 0/10-15 compilation tests (Phase 5)
- ✅ < 1 second total execution time (~150ms)
- ⏳ ~700/2000 lines of test code (35% complete)

### Qualitative ✅
- ✅ Features map to tests (coverage matrix complete)
- ✅ Easy to add tests (auto-discovery works)
- ✅ Easy to update tests (`update_goldens.sh` works)
- ✅ Clear failure messages (enhanced diff output)
- ✅ Tests document behavior (comments in .input files)
- ✅ Maintainable structure (feature-based organization)

---

## References

- **Master Plan**: `.claude/agents/test_restructure/test-restructuring-master-plan.md`
- **Phase 2 Review**: `.claude/agents/test_restructure/PHASE2_REVIEW_FINDINGS.md`
- **Golden File Agent**: `.claude/agents/test_restructure/golden-file-creator-agent.md`
- **Test Runner**: `tests/golden_ut.cpp`
- **Verification Tool**: `src/tools/verify_goldens.cpp`
- **Update Script**: `tests/tools/update_goldens.sh`

---

**Document Status**: ✅ Complete for Phase 3
**Last Updated**: Phase 3 completion
**Next Review**: After Phase 4 (Property Tests)
