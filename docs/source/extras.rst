Extras and Extension Data
=========================

Asset formats often contain application-specific metadata that AssetKit does
not need to interpret directly. COLLADA stores this as ``<extra>`` elements;
glTF stores it as ``extras`` and extension JSON objects.

AssetKit keeps this data as an ``AkTree`` so applications can inspect it
without linking against the source XML/JSON parser.

Reading Extras
--------------

Use :c:func:`ak_extra` with the object that owns the metadata:

.. code-block:: c

  AkTree *extra;

  extra = ak_extra(node);
  if (extra) {
    /* Walk extra->chld, node->next and node->attribs. */
  }

The returned tree is owned by the document heap. Do not free it directly.

For new objects or custom loader code, use :c:func:`ak_extra_set`:

.. code-block:: c

  ak_extra_set(object, extraTree);

COLLADA
-------

Historically, COLLADA loaders wrote ``<extra>`` data directly into struct
fields such as ``node->extra``, ``mesh->extra`` or ``material->extra``.
``ak_extra()`` is the public, generic entry point over that data. Existing
direct fields still work for old consumers, but new code should prefer
``ak_extra()`` when possible.

glTF
----

For glTF, AssetKit stores unhandled or inspectable JSON payload under a root
tree named ``extra``:

.. code-block:: text

  extra
    extensions
      KHR_example_extension
        enabled   type=value val=true
    extras
      author     type=value val=...

Each glTF tree node carries a ``type`` attribute. Current values are:

* ``object`` for JSON objects
* ``array`` for JSON arrays
* ``value`` for JSON scalar values
* ``null`` for null values
* ``unknown`` for unsupported parser node kinds

Array items are stored as repeated child nodes named ``item`` and preserve the
source order.

When to Use This
----------------

Use typed AssetKit fields for extensions that AssetKit understands, such as
material variants or Gaussian splat metadata. Use ``ak_extra()`` when you need
vendor-specific metadata, debug information, authoring-tool payloads or
extension data that AssetKit intentionally preserves but does not interpret.
