/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_fx_program.h"
#include "ak_collada_fx_shader.h"
#include "ak_collada_fx_uniform.h"
#include "ak_collada_fx_binary.h"

AkResult _assetkit_hide
ak_dae_fxProg(void * __restrict memParent,
               xmlTextReaderPtr reader,
               ak_program ** __restrict dest) {
  ak_program      *prog;
  ak_bind_uniform *last_bind_uniform;
  ak_bind_attrib  *last_bind_attrib;
  ak_linker       *last_linker;
  ak_shader       *last_shader;

  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  prog = ak_calloc(memParent, sizeof(*prog), 1);

  last_bind_uniform = NULL;
  last_bind_attrib  = NULL;
  last_linker       = NULL;
  last_shader       = NULL;

  do {
    _xml_beginElement(_s_dae_program);

    if (_xml_eqElm(_s_dae_shader)) {
      ak_shader *shader;
      int         ret;

      ret = ak_dae_fxShader(prog, reader, &shader);
      if (ret == AK_OK) {
        if (last_shader)
          last_shader->next = shader;
        else
          prog->shader = shader;

        last_shader = shader;
      }
    } else if (_xml_eqElm(_s_dae_linker)) {
      ak_linker *linker;
      ak_binary *last_binary;

      linker = ak_calloc(prog, sizeof(*linker), 1);

      _xml_readAttr(linker, linker->platform, _s_dae_platform);
      _xml_readAttr(linker, linker->target, _s_dae_target);
      _xml_readAttr(linker, linker->options, _s_dae_options);

      last_binary = NULL;

      do {
        _xml_beginElement(_s_dae_linker);

        if (_xml_eqElm(_s_dae_binary)) {
          ak_binary *binary;
          int         ret;

          ret = ak_dae_fxBinary(linker, reader, &binary);
          if (ret == AK_OK) {
            if (last_shader)
              last_binary->next = binary;
            else
              linker->binary = binary;

            last_binary = binary;
          }
        } else {
          _xml_skipElement;
        }

        /* end element */
        _xml_endElement;
      } while (nodeRet);

      if (last_linker)
        last_linker->next = linker;
      else
        prog->linker = linker;

      last_linker = linker;
    } else if (_xml_eqElm(_s_dae_bind_attribute)) {
      ak_bind_attrib *bind_attrib;

      bind_attrib = ak_calloc(prog, sizeof(*bind_attrib), 1);
      _xml_readAttr(bind_attrib, bind_attrib->symbol, _s_dae_symbol);

      do {
        _xml_beginElement(_s_dae_bind_attribute);

        if (_xml_eqElm(_s_dae_semantic)) {
          if (!bind_attrib->semantic)
            _xml_readText(bind_attrib, bind_attrib->semantic);
        } else {
          _xml_skipElement;
        }

        /* end element */
        _xml_endElement;
      } while (nodeRet);

      if (last_bind_attrib)
        last_bind_attrib->next = bind_attrib;
      else
        prog->bind_attrib = bind_attrib;

      last_bind_attrib = bind_attrib;
    } else if (_xml_eqElm(_s_dae_bind_uniform)) {
      ak_bind_uniform *bind_uniform;
      int ret;

      ret = ak_dae_fxBindUniform(prog, reader, &bind_uniform);
      if (ret == AK_OK) {
        if (last_bind_uniform)
          last_bind_uniform->next = bind_uniform;
        else
          prog->bind_uniform = bind_uniform;

        last_bind_uniform = bind_uniform;
      }
    } else {
      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);

  *dest = prog;
  
  return AK_OK;
}
