# Generated Header Guards

Atlas synthesizes header guards to prevent duplicate inclusion:

```
#ifndef <guard>
#define <guard>
...
#endif // <guard>
```

## Guard Construction

1. Start with `guard_prefix` if provided; otherwise, derive from the namespace and type name.
2. Replace namespace separators (`::`) with `guard_separator` (default `_`).
3. Append another separator and the SHA1 hash of the rendered class body.
4. Convert to uppercase if `upcase_guard` is `true`.

### Example

For:

- Namespace: `geom::measurement`
- Type name: `Distance`
- Guard separator: `_`
- Upcase: `true`

Atlas generates something like:

```
GEOM_MEASUREMENT_DISTANCE_7F5C1E6B...
```

The SHA1 digest ensures each generated header guard is unique to the code, preventing accidental collisions even when guard prefixes match.

## Interior Guards

Interaction headers carry a second layer of guards, one per generated construct,
named after what they protect:

| Guard | Protects |
| --- | --- |
| `ATLAS_INTERACTION_<sha1>` | a single interaction operator |
| `ATLAS_COMPOUND_ASSIGN_<op>` | the compound-assignment helpers for one operator |
| `ATLAS_VALUE_FOR_<sha1>` | an `atlas::atlas_value_for` accessor for one type |

These let independently generated interaction headers be included in the same
translation unit. Two modules that each need `Meters + Meters` can both declare
it; the second definition guards out. Without this, interactions would need a
hand-enforced "declare it in exactly one module" convention, which forces
unrelated modules to share an interaction file.

The `ATLAS_INTERACTION_` digest covers the complete operator definition, not just
its signature. Byte-identical definitions collapse into one. Two headers that
declare the same signature with *different* bodies — say, different value access
or one `constexpr` and one not — still collide, and the compiler diagnoses it
rather than the result depending on include order.

These guards are an implementation detail of the generated code. Do not define
them yourself.

## Customization Tips

- Use `--guard-prefix` to enforce project-wide guard formats (e.g., `MYPROJECT_STRONGTYPES`).
- Set `--guard-separator` to values like `_$_` if underscores conflict with naming conventions.
- Set `--upcase-guard=false` when integrating with codebases that prefer lowercase guards.
