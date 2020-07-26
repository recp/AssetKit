.. default-domain:: C

Options
===============================================================================

Currently **AssetKit** provides global options but in th future document based 
options may be supported.

Options / Preferences / Settings Design
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To make things simplier and easy to manage, **AssetKit** provides options as 
**key** - **value** pair. 

The key always is the :code:`AkOption` enum type. 
The value's type is :code:`uintptr_t`, so you can pass integers, enums and pointers (by casting to `uintptr_t` e.g. :code:`(uintptr_t)(void *)myPointer`).

Documented global options:

.. code-block:: c

    typedef enum AkOption {
      AK_OPT_INDICES_DEFAULT            = 0,  /* false    */
      AK_OPT_INDICES_SINGLE_INTERLEAVED = 1,  /* false    */
      AK_OPT_INDICES_SINGLE_SEPARATE    = 2,  /* false    */
      AK_OPT_INDICES_SINGLE             = 3,  /* false    */
      AK_OPT_NOINDEX_INTERLEAVED        = 4,  /* true     */
      AK_OPT_NOINDEX_SEPARATE           = 5,  /* true     */
      AK_OPT_COORD                      = 6,  /* Y_UP     */
      AK_OPT_DEFAULT_ID_PREFIX          = 7,  /* id-      */
      AK_OPT_COMPUTE_BBOX               = 8,  /* false    */
      AK_OPT_TRIANGULATE                = 9,  /* true     */
      AK_OPT_GEN_NORMALS_IF_NEEDED      = 10, /* true     */
      AK_OPT_DEFAULT_PROFILE            = 11, /* COMMON   */
      AK_OPT_EFFECT_PROFILE             = 12, /* true     */
      AK_OPT_TECHNIQUE                  = 13, /* "common" */
      AK_OPT_TECHNIQUE_FX               = 14, /* "common" */
      AK_OPT_ZERO_INDEXED_INPUT         = 15, /* false    */
      AK_OPT_IMAGE_LOAD_FLIP_VERTICALLY = 16, /* true     */
      AK_OPT_ADD_DEFAULT_CAMERA         = 17, /* true     */
      AK_OPT_ADD_DEFAULT_LIGHT          = 18, /* false    */
      AK_OPT_COORD_CONVERT_TYPE         = 19, /* DEFAULT  */
      AK_OPT_BUGFIXES                   = 20, /* TRUE     */
      AK_OPT_GLTF_EXT_SPEC_GLOSS        = 21, /* TRUE     */
      AK_OPT_COMPUTE_EXACT_CENTER       = 22  /* FALSE    */
    } AkOption;

The comment at right contains default value for that option. 
All options will be documented below. (**TODO**)

As you can see and imagine, not all options are integer or boolean.
For instance to set :code:`Y_UP` for :code:`AK_OPT_COORD` option, you need to cast :code:`Y_UP` opinter to :code:`uintptr_t` type.

Sample options:

.. code-block:: c

    /* compute bounding box for mesh and for its primitives */
    ak_opt_set(AK_OPT_COMPUTE_BBOX, true);

    /* triangulate meshes that are not triangles e.g. polygons... */
    ak_opt_set(AK_OPT_TRIANGULATE, true);

    /* change UP axis to Y UP, see coordinate sys documentation. */
    ak_opt_set(AK_OPT_TRIANGULATE, (uintptr_t)AK_YUP);

Functions:

1. :c:func:`ak_opt_set`
#. :c:func:`ak_opt_get`
#. :c:func:`ak_opt_set_str`

.. c:function:: void ak_opt_set(AkOption option, uintptr_t value)

    Sets an option by key and value. Pass integers, booleans or pointers as value. 
    Value needs to be casted to **uintptr_t**. Pass only supported values for a key.

    Parameters:
      | *[in]* **option** option (see AkOption enum)
      | *[in]* **value**  option value

.. c:function:: uintptr_t ak_opt_get(AkOption option)

    Get value of option as **uintptr_t**. If the option is pointer than you need to cast it to pointer. 
    For instance :code:`(AkCoordSys *)ak_opt_get(AK_OPT_COORD)` or :code:`(void *)ak_opt_get(AK_OPT_COORD)`.
    If there are no warnings then you don't need to cast result to Boolean or Integers for Boolean/Integer options.
    For instance :code:`if (ak_opt_get(AK_OPT_TRIANGULATE)) ...`

    Parameters:
      | *[in]* **option** option (see AkOption enum)

.. c:function:: void ak_opt_set_str(AkOption option, const char *value)

    Similar to :c:func:`ak_opt_set` but it accepts null terminated string parameter 
    and it uses :c:func:`ak_strdup` to duplicate string to keep it. 
    Then it casts duplicated string to :code:`uintptr_t`, so you can get value anytime with :c:func:`ak_opt_get`.

    **NOTE:** When you set new value then the old value will be free-ed. 
    If you need to keep old value then you must duplicate it yourself.
    Otherwise memory leaks would occoured...

    Parameters:
      | *[in]* **option** option (see AkOption enum)
      | *[in]* **value**  option value as null-terminated string
