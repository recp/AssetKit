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
ak_dae_fxShader(void * __restrict memParent,
                 xmlTextReaderPtr reader,
                 ak_shader ** __restrict dest) {
  ak_shader       *shader;
  ak_compiler     *last_compiler;
  ak_bind_uniform *last_bind_uniform;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  shader = ak_calloc(memParent, sizeof(*shader), 1);

  _xml_readAttrAsEnum(shader->stage,
                      _s_dae_stage,
                      ak_dae_fxEnumShaderStage);

  last_compiler     = NULL;
  last_bind_uniform = NULL;

  do {
    _xml_beginElement(_s_dae_shader);

    if (_xml_eqElm(_s_dae_sources)) {
      ak_sources *sources;
      ak_inline  *last_inline;
      ak_import  *last_import;

      sources = ak_calloc(shader, sizeof(*sources), 1);
      _xml_readAttr(sources, sources->entry, _s_dae_entry);

      last_inline = NULL;
      last_import = NULL;

      do {
        _xml_beginElement(_s_dae_sources);

        if (_xml_eqElm(_s_dae_inline)) {
          ak_inline *nInline;

          nInline = ak_calloc(shader, sizeof(*nInline), 1);
          _xml_readText(nInline, nInline->val);

          if (last_inline)
            last_inline->next = nInline;
          else
            sources->inlines = nInline;

          last_inline = nInline;
        } else if (_xml_eqElm(_s_dae_import)) {
          ak_import *nImport;

          nImport = ak_calloc(shader, sizeof(*nImport), 1);
          _xml_readText(nImport, nImport->ref);

          if (last_import)
            last_import->next = nImport;
          else
            sources->imports = nImport;

          last_import = nImport;
        } else {
          _xml_skipElement;
        }
        
        /* end element */
        _xml_endElement;
      } while (nodeRet);

      shader->sources = sources;
    } else if (_xml_eqElm(_s_dae_compiler)) {
      ak_compiler *compiler;

      compiler = ak_calloc(shader, sizeof(*compiler), 1);

      _xml_readAttr(compiler, compiler->platform, _s_dae_platform);
      _xml_readAttr(compiler, compiler->target, _s_dae_target);
      _xml_readAttr(compiler, compiler->options, _s_dae_options);

      do {
        _xml_beginElement(_s_dae_compiler);

        if (_xml_eqElm(_s_dae_binary)) {
          ak_binary *binary;
          AkResult   ret;

          ret = ak_dae_fxBinary(compiler, reader, &binary);
          if (ret == AK_OK)
            compiler->binary = binary;
        } else {
          _xml_skipElement;
        }

        /* end element */
        _xml_endElement;
      } while (nodeRet);

      if (last_compiler)
        last_compiler->next = compiler;
      else
        shader->compiler = compiler;

      last_compiler = compiler;
    } else if (_xml_eqElm(_s_dae_bind_uniform)) {
      ak_bind_uniform *bind_uniform;
      AkResult ret;

      ret = ak_dae_fxBindUniform(shader, reader, &bind_uniform);
      if (ret == AK_OK) {
        if (last_bind_uniform)
          last_bind_uniform->next = bind_uniform;
        else
          shader->bind_uniform = bind_uniform;

        last_bind_uniform = bind_uniform;
      }
    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(reader);
      tree = NULL;

      ak_tree_fromXmlNode(shader, nodePtr, &tree, NULL);
      shader->extra = tree;

      _xml_skipElement;
    } else {
      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);

  *dest = shader;
  
  return AK_OK;
}
