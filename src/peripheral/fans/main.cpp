#include <Arduino.h>

#include "FanController.h"
#include "FansNodeApp.h"
#include "RelayController.h"
#include "shared/mesh/MeshRegistry.h"

namespace {
    constexpr int kRelayPin = 7;
    constexpr int kFan1FgPin = 10;
    constexpr int kFan1PwmPin = 20;
    constexpr int kFan2FgPin = 21;
    constexpr int kFan2PwmPin = 9;
    constexpr gpio_num_t kStatusLedPin = GPIO_NUM_8;
    constexpr uint32_t kPwmFreq = 25000;
    constexpr uint8_t kPwmResolution = 8;
    constexpr bool kPwmInvert = false;

    FanController fan1("Front", kFan1FgPin, kFan1PwmPin, kPwmFreq, kPwmResolution, kPwmInvert);
    FanController fan2("Rear", kFan2FgPin, kFan2PwmPin, kPwmFreq, kPwmResolution, kPwmInvert);
    RelayController relay(kRelayPin);
    mesh::MeshRegistry registry;
    FansNodeApp app(fan1, fan2, relay, registry, kStatusLedPin);

    void IRAM_ATTR onFan1Pulse() {
        app.onFan1Pulse();
    }

    void IRAM_ATTR onFan2Pulse() {
        app.onFan2Pulse();
    }
} // namespace

void setup() {
    app.begin();
    attachInterrupt(digitalPinToInterrupt(kFan1FgPin), onFan1Pulse, FALLING);
    attachInterrupt(digitalPinToInterrupt(kFan2FgPin), onFan2Pulse, FALLING);
}

void loop() {
    app.loop(millis());
}
