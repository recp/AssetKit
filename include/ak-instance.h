/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_instance_h
#define ak_instance_h

AK_EXPORT
void *
ak_instanceObject(AkInstanceBase *instance);

AK_EXPORT
AkGeometry *
ak_instanceObjectGeom(AkNode * node);

AK_EXPORT
AkGeometry *
ak_instanceObjectGeomId(AkDoc * __restrict doc,
                        const char * id);

#endif /* ak_instance_h */
