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
ak_dae_fxProg(AkXmlState * __restrict xst,
              void * __restrict memParent,
              AkProgram ** __restrict dest) {
  AkProgram     *prog;
  AkBindUniform *last_bind_uniform;
  AkBindAttrib  *last_bind_attrib;
  AkLinker      *last_linker;
  AkShader      *last_shader;

  prog = ak_heap_calloc(xst->heap,
                        memParent,
                        sizeof(*prog),
                        false);

  last_bind_uniform = NULL;
  last_bind_attrib  = NULL;
  last_linker       = NULL;
  last_shader       = NULL;

  do {
    if (ak_xml_beginelm(xst, _s_dae_program))
      break;

    if (ak_xml_eqelm(xst, _s_dae_shader)) {
      AkShader *shader;
      AkResult  ret;

      ret = ak_dae_fxShader(xst, prog, &shader);
      if (ret == AK_OK) {
        if (last_shader)
          last_shader->next = shader;
        else
          prog->shader = shader;

        last_shader = shader;
      }
    } else if (ak_xml_eqelm(xst, _s_dae_linker)) {
      AkLinker *linker;
      AkBinary *last_binary;

      linker = ak_heap_calloc(xst->heap,
                              prog,
                              sizeof(*linker),
                              false);

      linker->platform = ak_xml_attr(xst, linker, _s_dae_platform);
      linker->target   = ak_xml_attr(xst, linker, _s_dae_target);
      linker->options  = ak_xml_attr(xst, linker, _s_dae_options);

      last_binary = NULL;

      do {
        if (ak_xml_beginelm(xst, _s_dae_linker))
          break;

        if (ak_xml_eqelm(xst, _s_dae_binary)) {
          AkBinary *binary;
          AkResult  ret;

          ret = ak_dae_fxBinary(xst, linker, &binary);
          if (ret == AK_OK) {
            if (last_shader)
              last_binary->next = binary;
            else
              linker->binary = binary;

            last_binary = binary;
          }
        } else {
          ak_xml_skipelm(xst);
        }

        /* end element */
        ak_xml_endelm(xst);
      } while (xst->nodeRet);

      if (last_linker)
        last_linker->next = linker;
      else
        prog->linker = linker;

      last_linker = linker;
    } else if (ak_xml_eqelm(xst, _s_dae_bind_attribute)) {
      AkBindAttrib *bindAttrib;

      bindAttrib = ak_heap_calloc(xst->heap,
                                  prog,
                                  sizeof(*bindAttrib),
                                  false);

      bindAttrib->symbol = ak_xml_attr(xst, bindAttrib, _s_dae_symbol);

      do {
        if (ak_xml_beginelm(xst, _s_dae_bind_attribute))
          break;

        if (ak_xml_eqelm(xst, _s_dae_semantic)) {
          if (!bindAttrib->semantic)
            bindAttrib->semantic = ak_xml_val(xst, bindAttrib);
        } else {
          ak_xml_skipelm(xst);
        }

        /* end element */
        ak_xml_endelm(xst);
      } while (xst->nodeRet);

      if (last_bind_attrib)
        last_bind_attrib->next = bindAttrib;
      else
        prog->bindAttrib = bindAttrib;

      last_bind_attrib = bindAttrib;
    } else if (ak_xml_eqelm(xst, _s_dae_bind_uniform)) {
      AkBindUniform *bindUniform;
      AkResult ret;

      ret = ak_dae_fxBindUniform(xst, prog, &bindUniform);
      if (ret == AK_OK) {
        if (last_bind_uniform)
          last_bind_uniform->next = bindUniform;
        else
          prog->bindUniform = bindUniform;

        last_bind_uniform = bindUniform;
      }
    } else {
      ak_xml_skipelm(xst);
    }

    /* end element */
    ak_xml_endelm(xst);
  } while (xst->nodeRet);

  *dest = prog;
  
  return AK_OK;
}
