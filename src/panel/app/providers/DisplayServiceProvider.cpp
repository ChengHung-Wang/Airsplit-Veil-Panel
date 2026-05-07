#include "app/providers/DisplayServiceProvider.h"

#include "config/lvgl_port_v8.h"

DisplayServiceProvider::DisplayServiceProvider(PanelApplicationContext &context) : context_(context) {
}

void DisplayServiceProvider::registerServices() {
    initializeDisplayPowerPin();

    Serial.println("Initialize panel device");
    context_.panel = new ESP_Panel();
    context_.panel->init();
#if LVGL_PORT_AVOID_TEAR
    ESP_PanelBus *lcdBus = context_.panel->getLcd()->getBus();
#if ESP_PANEL_LCD_BUS_TYPE == ESP_PANEL_BUS_TYPE_RGB
    static_cast<ESP_PanelBus_RGB *>(lcdBus)->configRgbBounceBufferSize(LVGL_PORT_RGB_BOUNCE_BUFFER_SIZE);
    static_cast<ESP_PanelBus_RGB *>(lcdBus)->configRgbFrameBufferNumber(LVGL_PORT_DISP_BUFFER_NUM);
#elif ESP_PANEL_LCD_BUS_TYPE == ESP_PANEL_BUS_TYPE_MIPI_DSI
    static_cast<ESP_PanelBus_DSI *>(lcdBus)->configDpiFrameBufferNumber(LVGL_PORT_DISP_BUFFER_NUM);
#endif
#endif
    context_.panel->begin();
}

void DisplayServiceProvider::boot() {
    Serial.println("Initialize LVGL");
    lvgl_port_init(context_.panel->getLcd(), context_.panel->getTouch());
}

void DisplayServiceProvider::initializeDisplayPowerPin() {
#ifdef BOARD_UEDX46460015_MD50E
    pinMode(17, OUTPUT);
    digitalWrite(17, HIGH);
#endif
}
