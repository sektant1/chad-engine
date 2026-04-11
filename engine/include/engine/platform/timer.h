
#pragma once

#include <engine/core/types.h>

namespace chad
{

void timerInit();
void timerUpdate();      // call once per frame
f64  timerGetDelta();    // seconds since last frame
f64  timerGetElapsed();  // seconds since init
u32  timerGetFPS();

}  // namespace chad
