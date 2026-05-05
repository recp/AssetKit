# Extras

AssetKit preserves source-format metadata that it does not need to interpret
directly.

- COLLADA: `<extra>`
- glTF: `extras` and preserved `extensions` payloads

Use `ak_extra()` to read it:

```c
AkTree *extra;

extra = ak_extra(object);
if (extra) {
  /* Walk extra->chld, node->next and node->attribs. */
}
```

Use `ak_extra_set()` when custom loader code wants to attach metadata:

```c
ak_extra_set(object, extraTree);
```

The returned `AkTree` is owned by the document heap. Do not free it directly.

## COLLADA

Older AssetKit code stored COLLADA `<extra>` data directly on struct fields
such as `node->extra`, `mesh->extra`, `geom->extra` or `material->extra`.
Those fields still work. New code should prefer `ak_extra(object)` so the same
path works for COLLADA and glTF.

## glTF

glTF metadata is stored under a root tree named `extra`:

```text
extra
  extensions
    KHR_example_extension
      enabled   type=value val=true
  extras
    author     type=value val=...
```

Each glTF tree node has a `type` attribute:

- `object`
- `array`
- `value`
- `null`
- `unknown`

Array items are repeated child nodes named `item` and preserve source order.

Typed AssetKit fields are still preferred for extensions AssetKit understands,
for example material variants or Gaussian splat metadata. `ak_extra()` is for
metadata, vendor payloads, debug payloads, and extension data that higher-level
tools may want to inspect.

More detail: [docs/source/extras.rst](docs/source/extras.rst)
