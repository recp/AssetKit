/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_fx_profile.h"
#include "../ak_collada_param.h"
#include "../ak_collada_annotate.h"
#include "../ak_collada_asset.h"
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
ak_dae_profile(void * __restrict memParent,
               xmlTextReaderPtr reader,
               AkProfile ** __restrict dest) {
  AkProfile     *profile;
  AkNewParam    *last_newparam;
  AkCode        *last_code;
  AkInclude     *last_inc;
  AkTechniqueFx *last_techfx;

  const ak_enumpair *found;
  const xmlChar *nodeName;
  int nodeType;
  int nodeRet;

  nodeName = xmlTextReaderConstName(reader);

  if (profileMapLen == 0) {
    profileMapLen = AK_ARRAY_LEN(profileMap);
    qsort(profileMap,
          profileMapLen,
          sizeof(profileMap[0]),
          ak_enumpair_cmp);
  }

  found = bsearch(nodeName,
                  profileMap,
                  profileMapLen,
                  sizeof(profileMap[0]),
                  ak_enumpair_cmp2);

  switch (found->val) {
    case AK_PROFILE_TYPE_COMMON:
      profile = ak_calloc(memParent, sizeof(AkProfileCommon), 1);
      break;
    case AK_PROFILE_TYPE_GLSL: {
      AkProfileGLSL *glslProfile;
      glslProfile = ak_calloc(memParent, sizeof(AkProfileGLSL), 1);

      _xml_readAttr(glslProfile, glslProfile->platform, _s_dae_platform);

      profile = &glslProfile->base;
      break;
    }
    case AK_PROFILE_TYPE_GLES2: {
      AkProfileGLES2 *gles2Profile;
      gles2Profile = ak_calloc(memParent, sizeof(AkProfileGLES2), 1);

      _xml_readAttr(gles2Profile, gles2Profile->language, _s_dae_language);
      _xml_readAttr(gles2Profile, gles2Profile->platforms, _s_dae_platforms);

      profile = &gles2Profile->base;
      break;
    }
    case AK_PROFILE_TYPE_GLES: {
      AkProfileGLES *glesProfile;
      glesProfile = ak_calloc(memParent, sizeof(AkProfileGLES), 1);

      _xml_readAttr(glesProfile, glesProfile->platform, _s_dae_platform);

      profile = &glesProfile->base;
      break;
    }
    case AK_PROFILE_TYPE_CG: {
      AkProfileCG *cgProfile;
      cgProfile = ak_calloc(memParent, sizeof(AkProfileGLES2), 1);

      _xml_readAttr(cgProfile, cgProfile->platform, _s_dae_platform);

      profile = &cgProfile->base;
      break;
    }
    case AK_PROFILE_TYPE_BRIDGE: {
      AkProfileBridge *bridgeProfile;
      bridgeProfile = ak_calloc(memParent, sizeof(AkProfileGLES2), 1);

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
  _xml_readAttr(profile, profile->id, _s_dae_id);

  last_newparam = NULL;
  last_code     = NULL;
  last_inc      = NULL;
  last_techfx   = NULL;

  do {
    _xml_beginElement(found->key);

    if (_xml_eqElm(_s_dae_asset)) {
      AkAssetInf *assetInf;
      AkResult ret;

      assetInf = NULL;
      ret = ak_dae_assetInf(profile, reader, &assetInf);
      if (ret == AK_OK)
        profile->inf = assetInf;
    } else if (_xml_eqElm(_s_dae_newparam)) {
      AkNewParam *newparam;
      AkResult    ret;

      ret = ak_dae_newparam(profile,
                             reader,
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

      ret = ak_dae_techniqueFx(profile, reader, &technique_fx);
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

      nodePtr = xmlTextReaderExpand(reader);
      tree = NULL;

      ak_tree_fromXmlNode(profile, nodePtr, &tree, NULL);
      profile->extra = tree;

      _xml_skipElement;
    } else if (_xml_eqElm(_s_dae_code)) {
      AkCode *code;

      code = ak_calloc(profile, sizeof(*code), 1);
      _xml_readAttr(code, code->sid, _s_dae_sid);
      _xml_readConstText(code->val);

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

      inc = ak_calloc(profile, sizeof(*inc), 1);
      _xml_readAttr(inc, inc->sid, _s_dae_sid);
      _xml_readAttr(inc, inc->url, _s_dae_url);

      if (!inc->url)
        _xml_readConstText(inc->url);

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
      _xml_skipElement;
    } 

    /* end element */
    _xml_endElement;
  } while (nodeRet);

  *dest = profile;

  return AK_OK;
err:
  return AK_ERR;
}
