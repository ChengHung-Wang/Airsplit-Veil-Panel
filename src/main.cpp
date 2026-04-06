#include <Arduino.h>
#include <Button.h>
#include <ESP_Knob.h>
#include <ESP_Panel_Library.h>

#include "app/AppController.h"
#include "input/InputRouter.h"
#include "lvgl_port_v8.h"

namespace {
    ESP_Panel *g_panel = nullptr;
    ESP_Knob *g_knob = nullptr;
    Button *g_button = nullptr;
    InputRouter *g_inputRouter = nullptr;
    AppController *g_app = nullptr;

    void onKnobLeftEventCallback(int count, void *usr_data)
    {
        (void)count;
        (void)usr_data;
        if (g_inputRouter != nullptr) {
            g_inputRouter->emitKnobRight();
        }
    }

    void onKnobRightEventCallback(int count, void *usr_data)
    {
        (void)count;
        (void)usr_data;
        if (g_inputRouter != nullptr) {
            g_inputRouter->emitKnobLeft();
        }
    }

    void onButtonSingleClick(void *button_handle, void *usr_data)
    {
        (void)button_handle;
        (void)usr_data;
        if (g_inputRouter != nullptr) {
            g_inputRouter->emitSelectPress();
        }
    }

    void onButtonLongPressStart(void *button_handle, void *usr_data)
    {
        (void)button_handle;
        (void)usr_data;
        if (g_inputRouter != nullptr) {
            g_inputRouter->emitPowerToggle();
        }
    }
}

void setup()
{
#ifdef BOARD_UEDX46460015_MD50E
    pinMode(17, OUTPUT);
    digitalWrite(17, HIGH);
#endif

    const String title = "Airsplit Veil Panel";
    Serial.begin(115200);
    Serial.println(title + " start");

    Serial.println("Initialize panel device");
    g_panel = new ESP_Panel();
    g_panel->init();
#if LVGL_PORT_AVOID_TEAR
    ESP_PanelBus *lcdBus = g_panel->getLcd()->getBus();
#if ESP_PANEL_LCD_BUS_TYPE == ESP_PANEL_BUS_TYPE_RGB
    static_cast<ESP_PanelBus_RGB *>(lcdBus)->configRgbBounceBufferSize(LVGL_PORT_RGB_BOUNCE_BUFFER_SIZE);
    static_cast<ESP_PanelBus_RGB *>(lcdBus)->configRgbFrameBufferNumber(LVGL_PORT_DISP_BUFFER_NUM);
#elif ESP_PANEL_LCD_BUS_TYPE == ESP_PANEL_BUS_TYPE_MIPI_DSI
    static_cast<ESP_PanelBus_DSI *>(lcdBus)->configDpiFrameBufferNumber(LVGL_PORT_DISP_BUFFER_NUM);
#endif
#endif
    g_panel->begin();

    Serial.println("Initialize knob device");
    g_knob = new ESP_Knob(GPIO_NUM_KNOB_PIN_A, GPIO_NUM_KNOB_PIN_B);
    g_knob->begin();
    g_knob->attachLeftEventCallback(onKnobLeftEventCallback);
    g_knob->attachRightEventCallback(onKnobRightEventCallback);

    Serial.println("Initialize button device");
    g_button = new Button(GPIO_BUTTON_PIN, false);
    g_button->attachSingleClickEventCb(&onButtonSingleClick, nullptr);
    g_button->attachLongPressStartEventCb(&onButtonLongPressStart, nullptr);

    Serial.println("Initialize LVGL");
    lvgl_port_init(g_panel->getLcd(), g_panel->getTouch());

    static InputRouter inputRouter;
    static AppController app(g_panel);
    g_inputRouter = &inputRouter;
    g_app = &app;

    Serial.println("Create application");
    g_app->begin();
    Serial.println(title + " ready");
}

void loop()
{
    if ((g_inputRouter == nullptr) || (g_app == nullptr)) {
        delay(10);
        return;
    }

    g_inputRouter->pollSerial(Serial, Serial);

    InputEvent event{};
    while (g_inputRouter->dequeue(event)) {
        g_app->handleEvent(event, millis());
    }

    g_app->update(millis());
    g_app->renderIfNeeded();
    delay(5);
}
