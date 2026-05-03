#include <Arduino.h>
#include <Button.h>
#include <ESP_Knob.h>
#include <ESP_Panel_Library.h>

#include "app/AppController.h"
#include "input/InputRouter.h"
#include "config/lvgl_port_v8.h"

#include <WiFi.h>
#include "esp_mac.h"

namespace {
    ESP_Panel *g_panel = nullptr;
    ESP_Knob *g_knob = nullptr;
    Button *g_button = nullptr;
    InputRouter *g_inputRouter = nullptr;
    AppController *g_app = nullptr;
    HardwareSerial g_uart1(1);

    constexpr uint32_t kUartCommandBaudRate = 115200;
    constexpr int kUart1RxPin = 44;
    constexpr int kUart1TxPin = 43;

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
    Serial.begin(kUartCommandBaudRate);
    g_uart1.begin(kUartCommandBaudRate, SERIAL_8N1, kUart1RxPin, kUart1TxPin);
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

    Serial.println("\n========================================");
    Serial.println("       ESP32-C3 硬體 MAC 地址清單       ");
    Serial.println("========================================");

    uint8_t mac[6];
    // 1. Wi-Fi Station (最常用於 ESP-NOW)
    if (esp_read_mac(mac, ESP_MAC_WIFI_STA) == ESP_OK) {
        Serial.printf("%-15s: %02X:%02X:%02X:%02X:%02X:%02X\r\n\r\n", 
                      "Wi-Fi STA", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }

    // 2. Wi-Fi SoftAP (熱點模式)
    if (esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP) == ESP_OK) {
        Serial.printf("%-15s: %02X:%02X:%02X:%02X:%02X:%02X\r\n\r\n", 
                      "Wi-Fi SoftAP", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }

    // 3. Bluetooth (BLE 模式)
    if (esp_read_mac(mac, ESP_MAC_BT) == ESP_OK) {
        Serial.printf("%-15s: %02X:%02X:%02X:%02X:%02X:%02X\r\n\r\n", 
                      "Bluetooth/BLE", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }

    // 4. Ethernet (雖然 C3 無實體硬體，但底層仍有分配空間)
    if (esp_read_mac(mac, ESP_MAC_ETH) == ESP_OK) {
        Serial.printf("%-15s: %02X:%02X:%02X:%02X:%02X:%02X\r\n\r\n", 
                      "Ethernet", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
    Serial.println("========================================\r\n\r\n");   
}

void loop()
{
    if ((g_inputRouter == nullptr) || (g_app == nullptr)) {
        delay(10);
        return;
    }

    g_inputRouter->pollSerial(Serial, Serial, InputEventSource::Uart0);
    g_inputRouter->pollSerial(g_uart1, g_uart1, InputEventSource::Uart1);

    InputEvent event{};
    while (g_inputRouter->dequeue(event)) {
        g_app->handleEvent(event, millis());
    }

    g_app->update(millis());
    g_app->renderIfNeeded();
}
