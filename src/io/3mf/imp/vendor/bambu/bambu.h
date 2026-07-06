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

#ifndef ak_3mf_vendor_bambu_h
#define ak_3mf_vendor_bambu_h

#include "../../internal.h"

AK_HIDE
void
ak_3mf_bambu_orca_parse_metadata(AK3MFImportState * __restrict st);

AK_HIDE
AkMaterial*
ak_3mf_bambu_orca_material_for_object(AK3MFImportState * __restrict st,
                                      uint32_t                      objectId);

AK_HIDE
uint32_t
ak_3mf_bambu_orca_extruder_for_object(AK3MFImportState * __restrict st,
                                      uint32_t                      objectId);

AK_HIDE
AkMaterial*
ak_3mf_bambu_orca_material_for_extruder(AK3MFImportState * __restrict st,
                                        uint32_t                      extruder);

#endif /* ak_3mf_vendor_bambu_h */
