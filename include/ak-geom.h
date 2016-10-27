/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_geom_h
#define ak_geom_h

/*!
 * @brief Total input count except VERTEX input
 *
 * returns primitive.inputCount - 1 + primitive.vertices.inputCount
 *
 * @return total input count
 */
AK_EXPORT
uint32_t
ak_meshInputCount(AkMesh * __restrict mesh);

#endif /* ak_geom_h */
