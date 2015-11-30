/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetio__aio_collada_asset__h_
#define __libassetio__aio_collada_asset__h_

#include "../../include/assetio.h"

#define _AIO_ASSET_LOAD_AS_DOCINF 1
#define _AIO_ASSET_LOAD_AS_ASTINF 2

typedef struct _xmlNode xmlNode;

/**
 * @brief loads COLLADA's an asset element from xmlNode
 * 
 * @discussion this function is used for load COLLADA's asset element to both
 * library asset_inf and doc_inf. Because doc_inf gets data from asset element
 * 
 * @return 0(success) or -1(fail)
 */
int _assetio_hide
aio_load_collada_asset(xmlNode * __restrict xml_node, ...);

/**
 * @brief This macro is used for call aio_load_collada_asset function. If there
 * are is no any asset element then it does not change the ASSET_INF
 */
#define _AIO_ASSET_LOAD_TO(NODE, ASSET_INF)                                   \
  do {                                                                        \
    aio_assetinf * asset_inf;                                                 \
    int            ret;                                                       \
                                                                              \
    ret = aio_load_collada_asset(NODE,                                        \
                                 _AIO_ASSET_LOAD_AS_ASTINF,                   \
                                 &asset_inf);                                 \
                                                                              \
    if (ret == 0)                                                             \
      ASSET_INF = asset_inf;                                                  \
  } while(0)

#endif /* __libassetio__aio_collada_asset__h_ */
