/*
 * Copyright (C) 2020 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "topology.h"
#include "semantic.h"
#include "../core/enum.h"
#include "../../../array.h"

#include <string.h>

static
bool
dae_brep_input_source_is_object(AkInput * __restrict inp) {
  const char *sem;
  size_t      len;

  if (!inp)
    return false;

  if ((uint32_t)inp->semantic == AK_INPUT_SEMANTIC_VERTEX)
    return true;

  sem = inp->semanticRaw;
  if (!sem)
    return false;

  len = strlen(sem);
  switch (sem[0]) {
    case 'C':
      return ak_str_eq_packed_fast(sem, len, DAE_BREP_SEM_CURVE, 5u)
             || ak_str_eq_packed_fast(sem, len, DAE_BREP_SEM_CURVE2D, 7u);
    case 'E':
      return ak_str_eq_packed_fast(sem, len, DAE_BREP_SEM_EDGE, 4u)
             || ak_str_eq_packed_fast(sem, len, DAE_BREP_SEM_EGDE, 4u);
    case 'F':
      return ak_str_eq_packed_fast(sem, len, DAE_BREP_SEM_FACE, 4u);
    case 'S':
      return ak_str_eq_packed_fast(sem, len, DAE_BREP_SEM_SURFACE, 7u)
             || ak_str_eq_packed_fast(sem, len, DAE_BREP_SEM_SHELL, 5u);
    case 'W':
      return ak_str_eq_packed_fast(sem, len, DAE_BREP_SEM_WIRE, 4u);
    default:
      break;
  }

  return false;
}

static
AkInput*
dae_brep_input(DAEState * __restrict dst,
               xml_t    * __restrict xml,
               void     * __restrict parent) {
  AkHeap  *heap;
  AkInput *inp;
  AkURL   *url;

  heap             = dst->heap;
  inp              = dae_input_new(heap, parent);
  inp->semanticRaw = dae_semanticRaw(DAE_XMLA8(xml, semantic),
                                     heap,
                                     inp,
                                     &inp->semantic);

  if (!inp->semanticRaw) {
    ak_free(inp);
    return NULL;
  }

  inp->indexOffset = xmla_u32(DAE_XMLA8(xml, offset), 0);
  inp->set         = xmla_u32(DAE_XMLA4(xml, set),    0);

  if (dae_brep_input_source_is_object(inp))
    return inp;

  url = DAE_URL_FROM(dst, xml, source, parent);
  rb_insert(dst->inputmap, inp, url);

  return inp;
}

AK_HIDE AkEdges*
dae_edges(DAEState * __restrict dst,
          xml_t    * __restrict xml,
          void     * __restrict memp) {
  AkHeap      *heap;
  AkEdges     *edges;
  const xml_t *sval;

  heap  = dst->heap;
  edges = ak_heap_calloc(heap, memp, sizeof(*edges));

  xmla_setid(xml, heap, edges);

  edges->name  = DAE_XMLA_STRDUP8(xml, heap, name, edges);
  edges->count = xmla_u32(DAE_XMLA8(xml, count), 0);

  xml = xml->val;
  while (xml) {
    if (DAE_XML_TAG_EQ(xml, input)) {
      AkInput *inp;

      if ((inp = dae_brep_input(dst, xml, edges))) {
        inp->next     = edges->input;
        edges->input  = inp;
        edges->inputCount++;
      }
    } else if (DAE_XML_TAG_EQ(xml, p) && (sval = xmls(xml))) {
      AkUIntArray *prims;
      AkResult     ret;
      
      ret = xml_strtoui_array(heap, edges, sval, &prims);
      if (ret == AK_OK)
        edges->primitives = prims;
    } else if (DAE_XML_TAG_EQ(xml, extra)) {
      edges->extra = tree_fromxml(heap, edges, xml);
    }
    xml = xml->next;
  }

  return edges;
}

AK_HIDE AkWires*
dae_wires(DAEState * __restrict dst,
          xml_t    * __restrict xml,
          void     * __restrict memp) {
  AkHeap      *heap;
  AkWires     *wires;
  const xml_t *sval;

  heap  = dst->heap;
  wires = ak_heap_calloc(heap, memp, sizeof(*wires));

  xmla_setid(xml, heap, wires);

  wires->name  = DAE_XMLA_STRDUP8(xml, heap, name, wires);
  wires->count = xmla_u32(DAE_XMLA8(xml, count), 0);

  xml = xml->val;
  while (xml) {
    if (DAE_XML_TAG_EQ(xml, input)) {
      AkInput *inp;

      if ((inp = dae_brep_input(dst, xml, wires))) {
        inp->next     = wires->input;
        wires->input  = inp;
        wires->inputCount++;
      }
    } else if (DAE_XML_TAG_EQ(xml, vcount) && (sval = xmls(xml))) {
      AkUIntArray *vcount;
      AkResult     ret;
      
      ret = xml_strtoui_array(heap, wires, sval, &vcount);
      if (ret == AK_OK)
        wires->vcount = vcount;
    } else if (DAE_XML_TAG_EQ(xml, p) && (sval = xmls(xml))) {
      AkUIntArray *prims;
      AkResult     ret;
      
      ret = xml_strtoui_array(heap, wires, sval, &prims);
      if (ret == AK_OK)
        wires->primitives = prims;
    } else if (DAE_XML_TAG_EQ(xml, extra)) {
      wires->extra = tree_fromxml(heap, wires, xml);
    }
    xml = xml->next;
  }

  return wires;
}

AK_HIDE AkFaces*
dae_faces(DAEState * __restrict dst,
          xml_t    * __restrict xml,
          void     * __restrict memp) {
  AkHeap      *heap;
  AkFaces     *faces;
  const xml_t *sval;

  heap  = dst->heap;
  faces = ak_heap_calloc(heap, memp, sizeof(*faces));

  xmla_setid(xml, heap, faces);

  faces->name  = DAE_XMLA_STRDUP8(xml, heap, name, faces);
  faces->count = xmla_u32(DAE_XMLA8(xml, count), 0);

  xml = xml->val;
  while (xml) {
    if (DAE_XML_TAG_EQ(xml, input)) {
      AkInput *inp;

      if ((inp = dae_brep_input(dst, xml, faces))) {
        inp->next     = faces->input;
        faces->input  = inp;
        faces->inputCount++;
      }
    } else if (DAE_XML_TAG_EQ(xml, vcount) && (sval = xmls(xml))) {
      AkUIntArray *vcount;
      AkResult     ret;
      
      ret = xml_strtoui_array(heap, faces, sval, &vcount);
      if (ret == AK_OK)
        faces->vcount = vcount;
    } else if (DAE_XML_TAG_EQ(xml, p) && (sval = xmls(xml))) {
      AkUIntArray *prims;
      AkResult     ret;
      
      ret = xml_strtoui_array(heap, faces, sval, &prims);
      if (ret == AK_OK)
        faces->primitives = prims;
    } else if (DAE_XML_TAG_EQ(xml, extra)) {
      faces->extra = tree_fromxml(heap, faces, xml);
    }
    xml = xml->next;
  }

  return faces;
}

AK_HIDE AkPCurves*
dae_pcurves(DAEState * __restrict dst,
            xml_t    * __restrict xml,
            void     * __restrict memp) {
  AkHeap      *heap;
  AkPCurves   *pcurves;
  const xml_t *sval;

  heap  = dst->heap;
  pcurves = ak_heap_calloc(heap, memp, sizeof(*pcurves));

  xmla_setid(xml, heap, pcurves);

  pcurves->name  = DAE_XMLA_STRDUP8(xml, heap, name, pcurves);
  pcurves->count = xmla_u32(DAE_XMLA8(xml, count), 0);

  xml = xml->val;
  while (xml) {
    if (DAE_XML_TAG_EQ(xml, input)) {
      AkInput *inp;

      if ((inp = dae_brep_input(dst, xml, pcurves))) {
        inp->next       = pcurves->input;
        pcurves->input  = inp;
        pcurves->inputCount++;
      }
    } else if (DAE_XML_TAG_EQ(xml, vcount) && (sval = xmls(xml))) {
      AkUIntArray *vcount;
      AkResult     ret;
      
      ret = xml_strtoui_array(heap, pcurves, sval, &vcount);
      if (ret == AK_OK)
        pcurves->vcount = vcount;
    } else if (DAE_XML_TAG_EQ(xml, p) && (sval = xmls(xml))) {
      AkUIntArray *prims;
      AkResult     ret;
      
      ret = xml_strtoui_array(heap, pcurves, sval, &prims);
      if (ret == AK_OK)
        pcurves->primitives = prims;
    } else if (DAE_XML_TAG_EQ(xml, extra)) {
      pcurves->extra = tree_fromxml(heap, pcurves, xml);
    }
    xml = xml->next;
  }

  return pcurves;
}

AK_HIDE AkShells*
dae_shells(DAEState * __restrict dst,
           xml_t    * __restrict xml,
           void     * __restrict memp) {
  AkHeap      *heap;
  AkShells    *shells;
  const xml_t *sval;

  heap  = dst->heap;
  shells = ak_heap_calloc(heap, memp, sizeof(*shells));

  xmla_setid(xml, heap, shells);

  shells->name  = DAE_XMLA_STRDUP8(xml, heap, name, shells);
  shells->count = xmla_u32(DAE_XMLA8(xml, count), 0);

  xml = xml->val;
  while (xml) {
    if (DAE_XML_TAG_EQ(xml, input)) {
      AkInput *inp;

      if ((inp = dae_brep_input(dst, xml, shells))) {
        inp->next      = shells->input;
        shells->input  = inp;
        shells->inputCount++;
      }
    } else if (DAE_XML_TAG_EQ(xml, vcount) && (sval = xmls(xml))) {
      AkUIntArray *vcount;
      AkResult     ret;
      
      ret = xml_strtoui_array(heap, shells, sval, &vcount);
      if (ret == AK_OK)
        shells->vcount = vcount;
    } else if (DAE_XML_TAG_EQ(xml, p) && (sval = xmls(xml))) {
      AkUIntArray *prims;
      AkResult     ret;
      
      ret = xml_strtoui_array(heap, shells, sval, &prims);
      if (ret == AK_OK)
        shells->primitives = prims;
    } else if (DAE_XML_TAG_EQ(xml, extra)) {
      shells->extra = tree_fromxml(heap, shells, xml);
    }
    xml = xml->next;
  }

  return shells;
}

AK_HIDE AkSolids*
dae_solids(DAEState * __restrict dst,
           xml_t    * __restrict xml,
           void     * __restrict memp){
  AkHeap      *heap;
  AkSolids    *solids;
  const xml_t *sval;

  heap  = dst->heap;
  solids = ak_heap_calloc(heap, memp, sizeof(*solids));

  xmla_setid(xml, heap, solids);

  solids->name  = DAE_XMLA_STRDUP8(xml, heap, name, solids);
  solids->count = xmla_u32(DAE_XMLA8(xml, count), 0);

  xml = xml->val;
  while (xml) {
    if (DAE_XML_TAG_EQ(xml, input)) {
      AkInput *inp;

      if ((inp = dae_brep_input(dst, xml, solids))) {
        inp->next      = solids->input;
        solids->input  = inp;
        solids->inputCount++;
      }
    } else if (DAE_XML_TAG_EQ(xml, vcount) && (sval = xmls(xml))) {
      AkUIntArray *vcount;
      AkResult     ret;
      
      ret = xml_strtoui_array(heap, solids, sval, &vcount);
      if (ret == AK_OK)
        solids->vcount = vcount;
    } else if (DAE_XML_TAG_EQ(xml, p) && (sval = xmls(xml))) {
      AkUIntArray *prims;
      AkResult     ret;
      
      ret = xml_strtoui_array(heap, solids, sval, &prims);
      if (ret == AK_OK)
        solids->primitives = prims;
    } else if (DAE_XML_TAG_EQ(xml, extra)) {
      solids->extra = tree_fromxml(heap, solids, xml);
    }
    xml = xml->next;
  }

  return solids;
}
