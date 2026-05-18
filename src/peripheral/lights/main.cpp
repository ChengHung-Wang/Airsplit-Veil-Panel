#include <Arduino.h>

#include "LightNodeApp.h"
#include "shared/mesh/MeshRegistry.h"

namespace {
    constexpr gpio_num_t kRelayPin = GPIO_NUM_11;
    constexpr gpio_num_t kStatusLedPin = GPIO_NUM_27;
    constexpr gpio_num_t kToggleButtonPin = GPIO_NUM_28;

    mesh::MeshRegistry registry;
    LightNodeApp app(registry, kRelayPin, kStatusLedPin, kToggleButtonPin);
} // namespace

void setup() {
    app.begin();
}

void loop() {
    app.loop(millis());
}
