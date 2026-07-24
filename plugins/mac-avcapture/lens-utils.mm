//
//  lens-utils.mm
//  mac-avcapture
//
//  Created by Sam Zhang on 2026/1/5.
//

//PRISM/sam.zhang/20251204/notify av capture error

#include "lens-utils.h"
#include <pls/pls-lens-event.h>

bool is_lens_active(int cam_index)
{
    return pls_is_lens_active(cam_index);
}
