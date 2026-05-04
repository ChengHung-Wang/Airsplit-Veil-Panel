#include <Arduino.h>

#include "LightNodeApp.h"
#include "shared/mesh/MeshRegistry.h"

namespace {

constexpr gpio_num_t kRelayPin = GPIO_NUM_8;

mesh::MeshRegistry registry;
LightNodeApp app(registry, kRelayPin);

}  // namespace

void setup()
{
    app.begin();
}

void loop()
{
    app.loop(millis());
}
