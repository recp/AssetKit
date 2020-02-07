/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "topology.h"
#include "../core/enum.h"
#include "../../array.h"

AkEdges* _assetkit_hide
dae_edges(DAEState * __restrict dst,
          xml_t    * __restrict xml,
          void     * __restrict memp) {
  AkHeap  *heap;
  AkEdges *edges;
  char    *sval;

  heap  = dst->heap;
  edges = ak_heap_calloc(heap, memp, sizeof(*edges));

  xmla_setid(xml, heap, edges);

  edges->name  = xmla_strdup_by(xml, heap, _s_dae_name, edges);
  edges->count = xmla_uint32(xml_attr(xml, _s_dae_count), 0);

  xml = xml->val;
  while (xml) {
    if (xml_tag_eq(xml, _s_dae_input)) {
      AkInput *inp;
      
      inp              = ak_heap_calloc(heap, edges, sizeof(*inp));
      inp->semanticRaw = xmla_strdup_by(xml, heap, _s_dae_semantic, inp);
      
      if (!inp->semanticRaw) {
        ak_free(inp);
      } else {
        AkEnum inputSemantic;
        
        inputSemantic = dae_enumInputSemantic(inp->semanticRaw);
        inp->semantic = inputSemantic;
        
        if (inputSemantic < 0)
          inputSemantic = AK_INPUT_SEMANTIC_OTHER;
        
        inp->semantic = inputSemantic;
        inp->offset   = xmla_uint32(xml_attr(xml, _s_dae_offset), 0);
        inp->set      = xmla_uint32(xml_attr(xml, _s_dae_set),    0);
        
        if ((uint32_t)inp->semantic != AK_INPUT_SEMANTIC_VERTEX) {
          AkURL *url;
          
          inp->semantic = dae_enumInputSemantic(inp->semanticRaw);
          
          inp->next     = edges->input;
          edges->input  = inp;
          edges->inputCount++;
    
          url = url_from(xml, _s_dae_source, edges);
          rb_insert(dst->inputmap, inp, url);
        } else {
          ak_free(inp);
        }
      }
    } else if (xml_tag_eq(xml, _s_dae_p) && (sval = xml->val)) {
      AkUIntArray *prims;
      AkResult     ret;
      
      ret = ak_strtoui_array(heap, edges, sval, &prims);
      if (ret == AK_OK)
        edges->primitives = prims;
    } else if (xml_tag_eq(xml, _s_dae_extra)) {
      edges->extra = tree_fromxml(heap, edges, xml);
    }
    xml = xml->next;
  }

  return edges;
}

AkWires* _assetkit_hide
dae_wires(DAEState * __restrict dst,
          xml_t    * __restrict xml,
          void     * __restrict memp) {
  AkHeap  *heap;
  AkWires *wires;
  char    *sval;

  heap  = dst->heap;
  wires = ak_heap_calloc(heap, memp, sizeof(*wires));

  xmla_setid(xml, heap, wires);

  wires->name  = xmla_strdup_by(xml, heap, _s_dae_name, wires);
  wires->count = xmla_uint32(xml_attr(xml, _s_dae_count), 0);

  xml = xml->val;
  while (xml) {
    if (xml_tag_eq(xml, _s_dae_input)) {
      AkInput *inp;
      
      inp              = ak_heap_calloc(heap, wires, sizeof(*inp));
      inp->semanticRaw = xmla_strdup_by(xml, heap, _s_dae_semantic, inp);
      
      if (!inp->semanticRaw) {
        ak_free(inp);
      } else {
        AkEnum inputSemantic;
        
        inputSemantic = dae_enumInputSemantic(inp->semanticRaw);
        inp->semantic = inputSemantic;
        
        if (inputSemantic < 0)
          inputSemantic = AK_INPUT_SEMANTIC_OTHER;
        
        inp->semantic = inputSemantic;
        inp->offset   = xmla_uint32(xml_attr(xml, _s_dae_offset), 0);
        inp->set      = xmla_uint32(xml_attr(xml, _s_dae_set),    0);
        
        if ((uint32_t)inp->semantic != AK_INPUT_SEMANTIC_VERTEX) {
          AkURL *url;
          
          inp->semantic = dae_enumInputSemantic(inp->semanticRaw);
          
          inp->next     = wires->input;
          wires->input  = inp;
          wires->inputCount++;
    
          url = url_from(xml, _s_dae_source, wires);
          rb_insert(dst->inputmap, inp, url);
        } else {
          ak_free(inp);
        }
      }
    } else if (xml_tag_eq(xml, _s_dae_vcount) && (sval = xml->val)) {
      AkUIntArray *vcount;
      AkResult     ret;
      
      ret = ak_strtoui_array(heap, wires, sval, &vcount);
      if (ret == AK_OK)
        wires->vcount = vcount;
    } else if (xml_tag_eq(xml, _s_dae_p) && (sval = xml->val)) {
      AkUIntArray *prims;
      AkResult     ret;
      
      ret = ak_strtoui_array(heap, wires, sval, &prims);
      if (ret == AK_OK)
        wires->primitives = prims;
    } else if (xml_tag_eq(xml, _s_dae_extra)) {
      wires->extra = tree_fromxml(heap, wires, xml);
    }
    xml = xml->next;
  }

  return wires;
}

AkFaces* _assetkit_hide
dae_faces(DAEState * __restrict dst,
          xml_t    * __restrict xml,
          void     * __restrict memp) {
  AkHeap  *heap;
  AkFaces *faces;
  char    *sval;

  heap  = dst->heap;
  faces = ak_heap_calloc(heap, memp, sizeof(*faces));

  xmla_setid(xml, heap, faces);

  faces->name  = xmla_strdup_by(xml, heap, _s_dae_name, faces);
  faces->count = xmla_uint32(xml_attr(xml, _s_dae_count), 0);

  xml = xml->val;
  while (xml) {
    if (xml_tag_eq(xml, _s_dae_input)) {
      AkInput *inp;
      
      inp              = ak_heap_calloc(heap, faces, sizeof(*inp));
      inp->semanticRaw = xmla_strdup_by(xml, heap, _s_dae_semantic, inp);
      
      if (!inp->semanticRaw) {
        ak_free(inp);
      } else {
        AkEnum inputSemantic;
        
        inputSemantic = dae_enumInputSemantic(inp->semanticRaw);
        inp->semantic = inputSemantic;
        
        if (inputSemantic < 0)
          inputSemantic = AK_INPUT_SEMANTIC_OTHER;
        
        inp->semantic = inputSemantic;
        inp->offset   = xmla_uint32(xml_attr(xml, _s_dae_offset), 0);
        inp->set      = xmla_uint32(xml_attr(xml, _s_dae_set),    0);
        
        if ((uint32_t)inp->semantic != AK_INPUT_SEMANTIC_VERTEX) {
          AkURL *url;
          
          inp->semantic = dae_enumInputSemantic(inp->semanticRaw);
          
          inp->next     = faces->input;
          faces->input  = inp;
          faces->inputCount++;
    
          url = url_from(xml, _s_dae_source, faces);
          rb_insert(dst->inputmap, inp, url);
        } else {
          ak_free(inp);
        }
      }
    } else if (xml_tag_eq(xml, _s_dae_vcount) && (sval = xml->val)) {
      AkUIntArray *vcount;
      AkResult     ret;
      
      ret = ak_strtoui_array(heap, faces, sval, &vcount);
      if (ret == AK_OK)
        faces->vcount = vcount;
    } else if (xml_tag_eq(xml, _s_dae_p) && (sval = xml->val)) {
      AkUIntArray *prims;
      AkResult     ret;
      
      ret = ak_strtoui_array(heap, faces, sval, &prims);
      if (ret == AK_OK)
        faces->primitives = prims;
    } else if (xml_tag_eq(xml, _s_dae_extra)) {
      faces->extra = tree_fromxml(heap, faces, xml);
    }
    xml = xml->next;
  }

  return faces;
}

AkPCurves* _assetkit_hide
dae_pcurves(DAEState * __restrict dst,
            xml_t    * __restrict xml,
            void     * __restrict memp) {
  AkHeap    *heap;
  AkPCurves *pcurves;
  char      *sval;

  heap  = dst->heap;
  pcurves = ak_heap_calloc(heap, memp, sizeof(*pcurves));

  xmla_setid(xml, heap, pcurves);

  pcurves->name  = xmla_strdup_by(xml, heap, _s_dae_name, pcurves);
  pcurves->count = xmla_uint32(xml_attr(xml, _s_dae_count), 0);

  xml = xml->val;
  while (xml) {
    if (xml_tag_eq(xml, _s_dae_input)) {
      AkInput *inp;
      
      inp              = ak_heap_calloc(heap, pcurves, sizeof(*inp));
      inp->semanticRaw = xmla_strdup_by(xml, heap, _s_dae_semantic, inp);
      
      if (!inp->semanticRaw) {
        ak_free(inp);
      } else {
        AkEnum inputSemantic;
        
        inputSemantic = dae_enumInputSemantic(inp->semanticRaw);
        inp->semantic = inputSemantic;
        
        if (inputSemantic < 0)
          inputSemantic = AK_INPUT_SEMANTIC_OTHER;
        
        inp->semantic = inputSemantic;
        inp->offset   = xmla_uint32(xml_attr(xml, _s_dae_offset), 0);
        inp->set      = xmla_uint32(xml_attr(xml, _s_dae_set),    0);
        
        if ((uint32_t)inp->semantic != AK_INPUT_SEMANTIC_VERTEX) {
          AkURL *url;
          
          inp->semantic = dae_enumInputSemantic(inp->semanticRaw);
          
          inp->next     = pcurves->input;
          pcurves->input  = inp;
          pcurves->inputCount++;
    
          url = url_from(xml, _s_dae_source, pcurves);
          rb_insert(dst->inputmap, inp, url);
        } else {
          ak_free(inp);
        }
      }
    } else if (xml_tag_eq(xml, _s_dae_vcount) && (sval = xml->val)) {
      AkUIntArray *vcount;
      AkResult     ret;
      
      ret = ak_strtoui_array(heap, pcurves, sval, &vcount);
      if (ret == AK_OK)
        pcurves->vcount = vcount;
    } else if (xml_tag_eq(xml, _s_dae_p) && (sval = xml->val)) {
      AkUIntArray *prims;
      AkResult     ret;
      
      ret = ak_strtoui_array(heap, pcurves, sval, &prims);
      if (ret == AK_OK)
        pcurves->primitives = prims;
    } else if (xml_tag_eq(xml, _s_dae_extra)) {
      pcurves->extra = tree_fromxml(heap, pcurves, xml);
    }
    xml = xml->next;
  }

  return pcurves;
}

AkShells* _assetkit_hide
dae_shells(DAEState * __restrict dst,
           xml_t    * __restrict xml,
           void     * __restrict memp) {
  AkHeap   *heap;
  AkShells *shells;
  char     *sval;

  heap  = dst->heap;
  shells = ak_heap_calloc(heap, memp, sizeof(*shells));

  xmla_setid(xml, heap, shells);

  shells->name  = xmla_strdup_by(xml, heap, _s_dae_name, shells);
  shells->count = xmla_uint32(xml_attr(xml, _s_dae_count), 0);

  xml = xml->val;
  while (xml) {
    if (xml_tag_eq(xml, _s_dae_input)) {
      AkInput *inp;
      
      inp              = ak_heap_calloc(heap, shells, sizeof(*inp));
      inp->semanticRaw = xmla_strdup_by(xml, heap, _s_dae_semantic, inp);
      
      if (!inp->semanticRaw) {
        ak_free(inp);
      } else {
        AkEnum inputSemantic;
        
        inputSemantic = dae_enumInputSemantic(inp->semanticRaw);
        inp->semantic = inputSemantic;
        
        if (inputSemantic < 0)
          inputSemantic = AK_INPUT_SEMANTIC_OTHER;
        
        inp->semantic = inputSemantic;
        inp->offset   = xmla_uint32(xml_attr(xml, _s_dae_offset), 0);
        inp->set      = xmla_uint32(xml_attr(xml, _s_dae_set),    0);
        
        if ((uint32_t)inp->semantic != AK_INPUT_SEMANTIC_VERTEX) {
          AkURL *url;
          
          inp->semantic = dae_enumInputSemantic(inp->semanticRaw);
          
          inp->next     = shells->input;
          shells->input  = inp;
          shells->inputCount++;
    
          url = url_from(xml, _s_dae_source, shells);
          rb_insert(dst->inputmap, inp, url);
        } else {
          ak_free(inp);
        }
      }
    } else if (xml_tag_eq(xml, _s_dae_vcount) && (sval = xml->val)) {
      AkUIntArray *vcount;
      AkResult     ret;
      
      ret = ak_strtoui_array(heap, shells, sval, &vcount);
      if (ret == AK_OK)
        shells->vcount = vcount;
    } else if (xml_tag_eq(xml, _s_dae_p) && (sval = xml->val)) {
      AkUIntArray *prims;
      AkResult     ret;
      
      ret = ak_strtoui_array(heap, shells, sval, &prims);
      if (ret == AK_OK)
        shells->primitives = prims;
    } else if (xml_tag_eq(xml, _s_dae_extra)) {
      shells->extra = tree_fromxml(heap, shells, xml);
    }
    xml = xml->next;
  }

  return shells;
}

AkSolids* _assetkit_hide
dae_solids(DAEState * __restrict dst,
           xml_t    * __restrict xml,
           void     * __restrict memp){
  AkHeap   *heap;
  AkSolids *solids;
  char     *sval;

  heap  = dst->heap;
  solids = ak_heap_calloc(heap, memp, sizeof(*solids));

  xmla_setid(xml, heap, solids);

  solids->name  = xmla_strdup_by(xml, heap, _s_dae_name, solids);
  solids->count = xmla_uint32(xml_attr(xml, _s_dae_count), 0);

  xml = xml->val;
  while (xml) {
    if (xml_tag_eq(xml, _s_dae_input)) {
      AkInput *inp;
      
      inp              = ak_heap_calloc(heap, solids, sizeof(*inp));
      inp->semanticRaw = xmla_strdup_by(xml, heap, _s_dae_semantic, inp);
      
      if (!inp->semanticRaw) {
        ak_free(inp);
      } else {
        AkEnum inputSemantic;
        
        inputSemantic = dae_enumInputSemantic(inp->semanticRaw);
        inp->semantic = inputSemantic;
        
        if (inputSemantic < 0)
          inputSemantic = AK_INPUT_SEMANTIC_OTHER;
        
        inp->semantic = inputSemantic;
        inp->offset   = xmla_uint32(xml_attr(xml, _s_dae_offset), 0);
        inp->set      = xmla_uint32(xml_attr(xml, _s_dae_set),    0);
        
        if ((uint32_t)inp->semantic != AK_INPUT_SEMANTIC_VERTEX) {
          AkURL *url;
          
          inp->semantic = dae_enumInputSemantic(inp->semanticRaw);
          
          inp->next     = solids->input;
          solids->input  = inp;
          solids->inputCount++;
    
          url = url_from(xml, _s_dae_source, solids);
          rb_insert(dst->inputmap, inp, url);
        } else {
          ak_free(inp);
        }
      }
    } else if (xml_tag_eq(xml, _s_dae_vcount) && (sval = xml->val)) {
      AkUIntArray *vcount;
      AkResult     ret;
      
      ret = ak_strtoui_array(heap, solids, sval, &vcount);
      if (ret == AK_OK)
        solids->vcount = vcount;
    } else if (xml_tag_eq(xml, _s_dae_p) && (sval = xml->val)) {
      AkUIntArray *prims;
      AkResult     ret;
      
      ret = ak_strtoui_array(heap, solids, sval, &prims);
      if (ret == AK_OK)
        solids->primitives = prims;
    } else if (xml_tag_eq(xml, _s_dae_extra)) {
      solids->extra = tree_fromxml(heap, solids, xml);
    }
    xml = xml->next;
  }

  return solids;
}
