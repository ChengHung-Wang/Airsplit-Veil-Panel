#include "PanelRuntime.h"

namespace {
    PanelRuntime g_runtime;
}

void setup() {
    g_runtime.setup();
}

void loop() {
    g_runtime.loop();
}
