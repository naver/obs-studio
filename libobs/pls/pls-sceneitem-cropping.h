#pragma once

#include "obs.h"
#ifdef __cplusplus
extern "C" {
#endif
EXPORT void pls_sceneitem_set_cropping(obs_sceneitem_t *item, bool cropping);

EXPORT bool pls_sceneitem_get_cropping(const obs_sceneitem_t *item);

#ifdef __cplusplus
}
#endif
