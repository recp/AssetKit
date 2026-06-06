/*
 * Copyright (C) 2026 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 */

#ifndef ak_instance_list_internal_h
#define ak_instance_list_internal_h

#include "../common.h"

AK_HIDE
void
ak_sceneAddItems(AkScene *scene, AkNode *node);

AK_HIDE
void
ak_sceneAddCamera(AkScene *scene, AkInstanceBase *inst);

AK_HIDE
void
ak_sceneAddLight(AkScene *scene, AkInstanceBase *inst);

#endif /* ak_instance_list_internal_h */
