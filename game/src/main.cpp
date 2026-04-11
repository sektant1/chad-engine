#include <engine/engine.h>
#include "engine/platform/input.h"
#include "engine/platform/timer.h"

using namespace chad;

int main()
{
    WindowConfig cfg {};
    Window      *window = windowCreate(cfg);

    inputInit(window);
    timerInit();

    while (!windowShouldClose(window)) {
        inputUpdate();
        timerUpdate();

        f64 delta_time = timerGetDelta();
        // LOG_INFO("Fps=%u delta=%.4f", timerGetFPS(), timerGetDelta());

        if (inputKeyPressed(Key::Escape)) {
            break;
        }

        if (inputKeyDown(Key::W)) {
            printf("W down\n");
        }

        windowSwapBuffers(window);
    }

    windowDestroy(window);
    return 0;
}
