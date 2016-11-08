/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_fx_profile.h"
#include "../core/ak_collada_param.h"
#include "../core/ak_collada_annotate.h"
#include "../core/ak_collada_asset.h"
#include "ak_collada_fx_technique.h"

static ak_enumpair profileMap[] = {
  {_s_dae_prfl_common, AK_PROFILE_TYPE_COMMON},
  {_s_dae_prfl_glsl,   AK_PROFILE_TYPE_GLSL},
  {_s_dae_prfl_gles2,  AK_PROFILE_TYPE_GLES2},
  {_s_dae_prfl_gles,   AK_PROFILE_TYPE_GLES},
  {_s_dae_prfl_cg,     AK_PROFILE_TYPE_CG},
  {_s_dae_prfl_bridge, AK_PROFILE_TYPE_BRIDGE}
};

static size_t profileMapLen = 0;

AkResult _assetkit_hide
ak_dae_profile(AkXmlState * __restrict xst,
               void * __restrict memParent,
               AkProfile ** __restrict dest) {
  AkProfile     *profile;
  AkNewParam    *last_newparam;
  AkCode        *last_code;
  AkInclude     *last_inc;
  AkTechniqueFx *last_techfx;

  const ak_enumpair *found;

  xst->nodeName = xmlTextReaderConstName(xst->reader);

  if (profileMapLen == 0) {
    profileMapLen = AK_ARRAY_LEN(profileMap);
    qsort(profileMap,
          profileMapLen,
          sizeof(profileMap[0]),
          ak_enumpair_cmp);
  }

  found = bsearch(xst->nodeName,
                  profileMap,
                  profileMapLen,
                  sizeof(profileMap[0]),
                  ak_enumpair_cmp2);

  switch (found->val) {
    case AK_PROFILE_TYPE_COMMON:
      profile = ak_heap_calloc(xst->heap,
                               memParent,
                               sizeof(AkProfileCommon),
                               true);
      break;
    case AK_PROFILE_TYPE_GLSL: {
      AkProfileGLSL *glslProfile;
      glslProfile = ak_heap_calloc(xst->heap,
                                   memParent,
                                   sizeof(AkProfileGLSL),
                                   true);

      _xml_readAttr(glslProfile, glslProfile->platform, _s_dae_platform);

      profile = &glslProfile->base;
      break;
    }
    case AK_PROFILE_TYPE_GLES2: {
      AkProfileGLES2 *gles2Profile;
      gles2Profile = ak_heap_calloc(xst->heap,
                                    memParent,
                                    sizeof(AkProfileGLES2),
                                    true);

      _xml_readAttr(gles2Profile, gles2Profile->language, _s_dae_language);
      _xml_readAttr(gles2Profile, gles2Profile->platforms, _s_dae_platforms);

      profile = &gles2Profile->base;
      break;
    }
    case AK_PROFILE_TYPE_GLES: {
      AkProfileGLES *glesProfile;
      glesProfile = ak_heap_calloc(xst->heap,
                                   memParent,
                                   sizeof(AkProfileGLES),
                                   true);

      _xml_readAttr(glesProfile, glesProfile->platform, _s_dae_platform);

      profile = &glesProfile->base;
      break;
    }
    case AK_PROFILE_TYPE_CG: {
      AkProfileCG *cgProfile;
      cgProfile = ak_heap_calloc(xst->heap,
                                 memParent,
                                 sizeof(AkProfileGLES2),
                                 true);

      _xml_readAttr(cgProfile, cgProfile->platform, _s_dae_platform);

      profile = &cgProfile->base;
      break;
    }
    case AK_PROFILE_TYPE_BRIDGE: {
      AkProfileBridge *bridgeProfile;
      bridgeProfile = ak_heap_calloc(xst->heap,
                                     memParent,
                                     sizeof(AkProfileGLES2),
                                     true);

      _xml_readAttr(bridgeProfile, bridgeProfile->platform, _s_dae_platform);
      _xml_readAttr(bridgeProfile, bridgeProfile->url, _s_dae_url);

      profile = &bridgeProfile->base;
      break;
    }
    default:
      goto err;
      break;
  }

  profile->profileType = found->val;

  _xml_readId(profile);

  last_newparam = NULL;
  last_code     = NULL;
  last_inc      = NULL;
  last_techfx   = NULL;

  do {
    if (ak_xml_beginelm(xst, found->key))
      break;

    if (_xml_eqElm(_s_dae_asset)) {
      AkAssetInf *assetInf;
      AkResult ret;

      assetInf = NULL;
      ret = ak_dae_assetInf(xst,
                            profile,
                            &assetInf);
      if (ret == AK_OK)
        profile->inf = assetInf;
    } else if (_xml_eqElm(_s_dae_newparam)) {
      AkNewParam *newparam;
      AkResult    ret;

      ret = ak_dae_newparam(xst,
                            profile,
                            &newparam);

      if (ret == AK_OK) {
        if (last_newparam)
          last_newparam->next = newparam;
        else
          profile->newparam = newparam;

        last_newparam = newparam;
      }
    } else if (_xml_eqElm(_s_dae_technique)) {
      AkTechniqueFx * technique_fx;
      AkResult ret;

      ret = ak_dae_techniqueFx(xst,
                               profile,
                               &technique_fx);
      if (ret == AK_OK) {
        if (last_techfx)
          last_techfx->next = technique_fx;
        else
          profile->technique = technique_fx;

        last_techfx = technique_fx;
      }

    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree    *tree;

      nodePtr = xmlTextReaderExpand(xst->reader);
      tree = NULL;

      ak_tree_fromXmlNode(xst->heap,
                          profile,
                          nodePtr,
                          &tree,
                          NULL);
      profile->extra = tree;

      ak_xml_skipelm(xst);;
    } else if (_xml_eqElm(_s_dae_code)) {
      AkCode *code;

      code = ak_heap_calloc(xst->heap,
                            profile,
                            sizeof(*code),
                            false);
      _xml_readAttr(code, code->sid, _s_dae_sid);
      code->val = ak_xml_val(xst, code);

      if (last_code) {
        last_code->next = code;
      } else {
        switch (found->val) {
          case AK_PROFILE_TYPE_GLSL:
            ((AkProfileGLSL *)profile)->code = code;
            break;
          case AK_PROFILE_TYPE_GLES2:
            ((AkProfileGLES2 *)profile)->code = code;
            break;
          case AK_PROFILE_TYPE_CG:
            ((AkProfileCG *)profile)->code = code;
            break;
          default:
            ak_free(code);
            code = last_code;
            break;
        }
      }

      last_code = code;

    } else if (_xml_eqElm(_s_dae_include)) {
      AkInclude *inc;

      inc = ak_heap_calloc(xst->heap,
                           profile,
                           sizeof(*inc),
                           false);
      _xml_readAttr(inc, inc->sid, _s_dae_sid);
      _xml_readAttr(inc, inc->url, _s_dae_url);

      if (!inc->url)
        inc->url = ak_xml_val(xst, inc);

      if (last_inc) {
        last_inc->next = inc;
      } else {
        switch (found->val) {
          case AK_PROFILE_TYPE_GLSL:
            ((AkProfileGLSL *)profile)->include = inc;
            break;
          case AK_PROFILE_TYPE_GLES2:
            ((AkProfileGLES2 *)profile)->include = inc;
            break;
          case AK_PROFILE_TYPE_CG:
            ((AkProfileCG *)profile)->include = inc;
            break;
          default:
            ak_free(inc);
            inc = last_inc;
            break;
        }
      }

      last_inc = inc;
    } else {
      ak_xml_skipelm(xst);;
    } 
    
    /* end element */
    ak_xml_endelm(xst);;
  } while (xst->nodeRet);
  
  *dest = profile;
  
  return AK_OK;
err:
  return AK_ERR;
}
