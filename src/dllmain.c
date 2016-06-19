/*
* Copyright (c), Recep Aslantas.
*
* MIT License (MIT), http://opensource.org/licenses/MIT
* Full license can be found in the LICENSE file
*/

#include "config.h"
#include "ak_memory_common.h"

BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD  ul_reason_for_call,
                      LPVOID lpReserved
)
{
  switch (ul_reason_for_call) {
  case DLL_PROCESS_ATTACH:
    ak__init();
    break;
  case DLL_THREAD_ATTACH:
    break;
  case DLL_THREAD_DETACH:
    break;
  case DLL_PROCESS_DETACH:
    ak__cleanup();
    break;
  }
  return TRUE;
}

