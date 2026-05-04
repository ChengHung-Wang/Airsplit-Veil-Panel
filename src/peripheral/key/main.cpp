#include <Arduino.h>

#include "KeyNodeApp.h"
#include "shared/mesh/MeshRegistry.h"

namespace {

constexpr gpio_num_t kPowerPin = GPIO_NUM_21;
constexpr gpio_num_t kWaterPin = GPIO_NUM_22;
constexpr gpio_num_t kLightPin = GPIO_NUM_23;
constexpr gpio_num_t kWindPin = GPIO_NUM_16;
constexpr bool kButtonActiveLevel = true;

mesh::MeshRegistry registry;
KeyNodeApp app(registry, kPowerPin, kWaterPin, kLightPin, kWindPin, kButtonActiveLevel);

}  // namespace

void setup()
{
    app.begin();
}

void loop()
{
    app.loop(millis());
}
