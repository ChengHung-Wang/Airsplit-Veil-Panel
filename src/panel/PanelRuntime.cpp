#include "PanelRuntime.h"

#include "config/lvgl_port_v8.h"

PanelRuntime *PanelRuntime::instance_ = nullptr;

PanelRuntime::MeshListener::MeshListener(PanelRuntime &runtime) : runtime_(runtime) {
}

void PanelRuntime::MeshListener::onMeshMessageReceived(const uint8_t mac[6], const mesh::MeshMessage &message) {
    runtime_.onMeshMessageReceived(mac, message, millis());
}

void PanelRuntime::MeshListener::onMeshSendComplete(const uint8_t mac[6], bool success) {
    runtime_.onMeshSendComplete(mac, success);
}

PanelRuntime::PanelRuntime() : meshListener_(*this),
                               network_(registry_, mesh::NodeRole::Panel, 1, meshListener_) {
}

void PanelRuntime::setup() {
    instance_ = this;

    initializeDisplayPowerPin();
    initializeSerial();
    initializePanel();
    initializeKnob();
    initializeButton();
    initializeLvgl();
    initializeMesh();
    initializeApplication();

    initialized_ = true;
}

void PanelRuntime::loop() {
    if (!initialized_ || (app_ == nullptr)) {
        delay(10);
        return;
    }

    const uint32_t nowMs = millis();
    processInput(nowMs);
    network_.poll(nowMs);
    app_->update(nowMs);
    app_->renderIfNeeded();
}

void PanelRuntime::initializeDisplayPowerPin() {
#ifdef BOARD_UEDX46460015_MD50E
    pinMode(17, OUTPUT);
    digitalWrite(17, HIGH);
#endif
}

void PanelRuntime::initializeSerial() {
    static const String kTitle = "Airsplit Veil Panel";

    Serial.begin(kUartCommandBaudRate);
    uart1_.begin(kUartCommandBaudRate, SERIAL_8N1, kUart1RxPin, kUart1TxPin);
    Serial.println(kTitle + " start");
}

void PanelRuntime::initializePanel() {
    Serial.println("Initialize panel device");
    panel_ = new ESP_Panel();
    panel_->init();
#if LVGL_PORT_AVOID_TEAR
    ESP_PanelBus *lcdBus = panel_->getLcd()->getBus();
#if ESP_PANEL_LCD_BUS_TYPE == ESP_PANEL_BUS_TYPE_RGB
    static_cast<ESP_PanelBus_RGB *>(lcdBus)->configRgbBounceBufferSize(LVGL_PORT_RGB_BOUNCE_BUFFER_SIZE);
    static_cast<ESP_PanelBus_RGB *>(lcdBus)->configRgbFrameBufferNumber(LVGL_PORT_DISP_BUFFER_NUM);
#elif ESP_PANEL_LCD_BUS_TYPE == ESP_PANEL_BUS_TYPE_MIPI_DSI
    static_cast<ESP_PanelBus_DSI *>(lcdBus)->configDpiFrameBufferNumber(LVGL_PORT_DISP_BUFFER_NUM);
#endif
#endif
    panel_->begin();
}

void PanelRuntime::initializeKnob() {
    Serial.println("Initialize knob device");
    knob_ = new ESP_Knob(GPIO_NUM_KNOB_PIN_A, GPIO_NUM_KNOB_PIN_B);
    knob_->begin();
    knob_->attachLeftEventCallback(&PanelRuntime::handleKnobLeftEvent);
    knob_->attachRightEventCallback(&PanelRuntime::handleKnobRightEvent);
}

void PanelRuntime::initializeButton() {
    Serial.println("Initialize button device");
    button_ = new Button(GPIO_BUTTON_PIN, false);
    button_->attachSingleClickEventCb(&PanelRuntime::handleButtonSingleClick, nullptr);
    button_->attachLongPressStartEventCb(&PanelRuntime::handleButtonLongPressStart, nullptr);
}

void PanelRuntime::initializeLvgl() {
    Serial.println("Initialize LVGL");
    lvgl_port_init(panel_->getLcd(), panel_->getTouch());
}

void PanelRuntime::initializeMesh() {
    Serial.println("Initialize ESP-NOW");
    if (!network_.begin()) {
        Serial.println("ESP-NOW init failed");
    }
}

void PanelRuntime::initializeApplication() {
    static const String kTitle = "Airsplit Veil Panel";

    Serial.println("Create application");
    app_ = new AppController(panel_, registry_, network_, mesh::NodeRole::Panel, 1);
    app_->begin();
    Serial.println(kTitle + " ready");
}

void PanelRuntime::processInput(uint32_t nowMs) {
    inputRouter_.pollSerial(Serial, Serial, InputEventSource::Uart0);
    inputRouter_.pollSerial(uart1_, uart1_, InputEventSource::Uart1);

    InputEvent event{};
    while (inputRouter_.dequeue(event)) {
        if (event.type == InputEventType::StatusRequest) {
            app_->printStatus(Serial, nowMs);
            app_->printStatus(uart1_, nowMs);
            continue;
        }
        app_->handleEvent(event, nowMs);
    }
}

void PanelRuntime::onMeshMessageReceived(const uint8_t mac[6], const mesh::MeshMessage &message, uint32_t nowMs) {
    if (app_ != nullptr) {
        app_->handleMeshMessage(mac, message, nowMs);
    }
}

void PanelRuntime::onMeshSendComplete(const uint8_t mac[6], bool success) {
    if (app_ != nullptr) {
        app_->handleMeshSendComplete(mac, success);
    }
}

void PanelRuntime::onKnobLeft() {
    inputRouter_.emitKnobRight();
}

void PanelRuntime::onKnobRight() {
    inputRouter_.emitKnobLeft();
}

void PanelRuntime::onButtonSingleClick() {
    inputRouter_.emitSelectPress();
}

void PanelRuntime::onButtonLongPressStart() {
    inputRouter_.emitPowerOff();
}

void PanelRuntime::handleKnobLeftEvent(int count, void *usrData) {
    (void) count;
    (void) usrData;
    if (instance_ != nullptr) {
        instance_->onKnobLeft();
    }
}

void PanelRuntime::handleKnobRightEvent(int count, void *usrData) {
    (void) count;
    (void) usrData;
    if (instance_ != nullptr) {
        instance_->onKnobRight();
    }
}

void PanelRuntime::handleButtonSingleClick(void *buttonHandle, void *usrData) {
    (void) buttonHandle;
    (void) usrData;
    if (instance_ != nullptr) {
        instance_->onButtonSingleClick();
    }
}

void PanelRuntime::handleButtonLongPressStart(void *buttonHandle, void *usrData) {
    (void) buttonHandle;
    (void) usrData;
    if (instance_ != nullptr) {
        instance_->onButtonLongPressStart();
    }
}
