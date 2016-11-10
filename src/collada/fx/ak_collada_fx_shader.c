/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_fx_shader.h"
#include "ak_collada_fx_enums.h"
#include "ak_collada_fx_binary.h"
#include "ak_collada_fx_uniform.h"

AkResult _assetkit_hide
ak_dae_fxShader(AkXmlState * __restrict xst,
                void * __restrict memParent,
                AkShader ** __restrict dest) {
  AkShader      *shader;
  AkCompiler    *last_compiler;
  AkBindUniform *last_bind_uniform;

  shader = ak_heap_calloc(xst->heap,
                          memParent,
                          sizeof(*shader),
                          false);

  shader->stage = ak_xml_attrenum(xst,
                                  _s_dae_stage,
                                  ak_dae_fxEnumShaderStage);

  last_compiler     = NULL;
  last_bind_uniform = NULL;

  do {
    if (ak_xml_beginelm(xst, _s_dae_shader))
      break;

    if (ak_xml_eqelm(xst, _s_dae_sources)) {
      AkSources *sources;
      AkInline  *last_inline;
      AkImport  *last_import;

      sources = ak_heap_calloc(xst->heap,
                               shader,
                               sizeof(*sources),
                               false);
      sources->entry = ak_xml_attr(xst, sources, _s_dae_entry);

      last_inline = NULL;
      last_import = NULL;

      do {
        if (ak_xml_beginelm(xst, _s_dae_sources))
          break;

        if (ak_xml_eqelm(xst, _s_dae_inline)) {
          AkInline *nInline;

          nInline = ak_heap_calloc(xst->heap,
                                   shader,
                                   sizeof(*nInline),
                                   false);
           nInline->val = ak_xml_val(xst, nInline);

          if (last_inline)
            last_inline->next = nInline;
          else
            sources->inlines = nInline;

          last_inline = nInline;
        } else if (ak_xml_eqelm(xst, _s_dae_import)) {
          AkImport *nImport;

          nImport = ak_heap_calloc(xst->heap,
                                   shader,
                                   sizeof(*nImport),
                                   false);
          nImport->ref = ak_xml_val(xst, nImport);

          if (last_import)
            last_import->next = nImport;
          else
            sources->imports = nImport;

          last_import = nImport;
        } else {
          ak_xml_skipelm(xst);
        }
        
        /* end element */
        ak_xml_endelm(xst);
      } while (xst->nodeRet);

      shader->sources = sources;
    } else if (ak_xml_eqelm(xst, _s_dae_compiler)) {
      AkCompiler *compiler;

      compiler = ak_heap_calloc(xst->heap,
                                shader,
                                sizeof(*compiler),
                                false);

      compiler->platform = ak_xml_attr(xst, compiler, _s_dae_platform);
      compiler->target   = ak_xml_attr(xst, compiler, _s_dae_target);
      compiler->options  = ak_xml_attr(xst, compiler, _s_dae_options);

      do {
        if (ak_xml_beginelm(xst, _s_dae_compiler))
          break;

        if (ak_xml_eqelm(xst, _s_dae_binary)) {
          AkBinary *binary;
          AkResult  ret;

          ret = ak_dae_fxBinary(xst, compiler, &binary);
          if (ret == AK_OK)
            compiler->binary = binary;
        } else {
          ak_xml_skipelm(xst);
        }

        /* end element */
        ak_xml_endelm(xst);
      } while (xst->nodeRet);

      if (last_compiler)
        last_compiler->next = compiler;
      else
        shader->compiler = compiler;

      last_compiler = compiler;
    } else if (ak_xml_eqelm(xst, _s_dae_bind_uniform)) {
      AkBindUniform *bindUniform;
      AkResult ret;

      ret = ak_dae_fxBindUniform(xst, shader, &bindUniform);
      if (ret == AK_OK) {
        if (last_bind_uniform)
          last_bind_uniform->next = bindUniform;
        else
          shader->bindUniform = bindUniform;

        last_bind_uniform = bindUniform;
      }
    } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(xst->reader);
      tree = NULL;

      ak_tree_fromXmlNode(xst->heap,
                          shader,
                          nodePtr,
                          &tree,
                          NULL);
      shader->extra = tree;

      ak_xml_skipelm(xst);
    } else {
      ak_xml_skipelm(xst);
    }

    /* end element */
    ak_xml_endelm(xst);
  } while (xst->nodeRet);

  *dest = shader;
  
  return AK_OK;
}
