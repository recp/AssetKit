/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_cam_h
#define ak_cam_h
#ifdef __cplusplus
extern "C" {
#endif

/*!
 * @brief Top camera in scene if available
 *
 * pass NULL which parameter is not wanted and reduce overhead of calc these 
 * params, only pass param[s] which you need
 *
 * @warning the returned projection matrix's aspect ratio is same as exported
 *
 * @param[in]  doc        document (required)
 * @param[out] camera     found camera (optional)
 * @param[out] matrix     transform matrix (optional) of camera in world
 * @param[out] projMatrix proj matrix (optional)
 *
 * @return AK_OK if camera found else AK_EFOUND
 */
AK_EXPORT
AkResult
ak_camFirstCamera(AkDoc     * __restrict doc,
                  AkCamera ** camera,
                  float     * matrix,
                  float     * projMatrix);

#ifdef __cplusplus
}
#endif
#endif /* ak_cam_h */
