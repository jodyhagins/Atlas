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

## Customization Tips

- Use `--guard-prefix` to enforce project-wide guard formats (e.g., `MYPROJECT_STRONGTYPES`).
- Set `--guard-separator` to values like `_$_` if underscores conflict with naming conventions.
- Set `--upcase-guard=false` when integrating with codebases that prefer lowercase guards.
