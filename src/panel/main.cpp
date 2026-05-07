#include "app/PanelApplication.h"

namespace {
    PanelApplication g_app;
}

void setup() {
    g_app.boot();
}

void loop() {
    g_app.runOnce();
}
