#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

// =========================
// User Config
// =========================
static const int RELAY_PIN = 7;

// Front fan
static const int FAN1_FG_PIN = 10; // blue line
static const int FAN1_PWM_PIN = 20;

// Rear fan
static const int FAN2_FG_PIN = 21; // white line
static const int FAN2_PWM_PIN = 9;

// PWM config
static const uint32_t PWM_FREQ = 25000; // 25kHz
static const uint8_t PWM_RES = 8;       // 8-bit, 0~255

// 若你的外部 NPN/NMOS 讓邏輯反相，改成 true
static const bool PWM_INVERT = false;

// 多數風扇是每轉 2 pulse，若不同請修改
static const uint8_t PULSES_PER_REV = 2;

// =========================
// BLE UART UUID (Nordic UART style)
// =========================
#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

class FanController;
static FanController *g_fan1Instance = nullptr;
static FanController *g_fan2Instance = nullptr;

// =========================
// FanController
// =========================
class FanController
{
private:
    String _name;
    int _fgPin;
    int _pwmPin;

    volatile uint32_t _pulseCount = 0;
    uint32_t _rpm = 0;
    int _dutyPercent = 0;

public:
    FanController(const String &name, int fgPin, int pwmPin)
        : _name(name), _fgPin(fgPin), _pwmPin(pwmPin) {}

    void begin()
    {
        pinMode(_fgPin, INPUT_PULLUP);
        ledcAttach(_pwmPin, PWM_FREQ, PWM_RES);
        writeDuty(0);
    }

    void onPulse()
    {
        _pulseCount += 1;
    }

    void writeDuty(int percent)
    {
        _dutyPercent = constrain(percent, 0, 100);

        int pwmValue = map(_dutyPercent, 0, 100, 0, 255);
        if (PWM_INVERT)
        {
            pwmValue = 255 - pwmValue;
        }

        ledcWrite(_pwmPin, pwmValue);
    }

    void updateRpmEverySecond()
    {
        noInterrupts();
        uint32_t pulses = _pulseCount;
        _pulseCount = 0;
        interrupts();

        _rpm = (pulses * 60) / PULSES_PER_REV;
    }

    String getName() const
    {
        return _name;
    }

    int getDuty() const
    {
        return _dutyPercent;
    }

    uint32_t getRpm() const
    {
        return _rpm;
    }
};

// =========================
// RelayController
// =========================
class RelayController
{
private:
    int _pin;
    bool _enabled = false;

public:
    RelayController(int pin) : _pin(pin) {}

    void begin()
    {
        pinMode(_pin, OUTPUT);
        disable();
    }

    void enable()
    {
        _enabled = true;
        digitalWrite(_pin, HIGH);
    }

    void disable()
    {
        _enabled = false;
        digitalWrite(_pin, LOW);
    }

    bool isEnabled() const
    {
        return _enabled;
    }
};

// =========================
// App
// =========================
class App
{
private:
    FanController &_fan1;
    FanController &_fan2;
    RelayController &_relay;

    BLECharacteristic *_txCharacteristic = nullptr;
    bool _bleConnected = false;
    uint32_t _lastRpmMs = 0;

public:
    App(FanController &fan1, FanController &fan2, RelayController &relay)
        : _fan1(fan1), _fan2(fan2), _relay(relay) {}

    void setBleConnected(bool connected)
    {
        _bleConnected = connected;
    }

    void setTxCharacteristic(BLECharacteristic *c)
    {
        _txCharacteristic = c;
    }

    void begin()
    {
        Serial.begin(115200);
        delay(300);

        _fan1.begin();
        _fan2.begin();
        _relay.begin();

        setupBle();

        printHelp();
        send("System Ready");
    }

    void loop()
    {
        handleSerial();
        handleRpmUpdate();
    }

    void processCommand(String cmd)
    {
        cmd.trim();
        cmd.toUpperCase();

        if (cmd.length() == 0)
            return;

        if (cmd == "HELP")
        {
            printHelp();
            return;
        }

        if (cmd == "STATUS")
        {
            send(statusString());
            return;
        }

        if (cmd == "RELAY=1" || cmd == "ENABLE")
        {
            _fan1.writeDuty(_fan1.getDuty());
            _fan2.writeDuty(_fan2.getDuty());
            _relay.enable();
            send("OK RELAY=ON");
            return;
        }

        if (cmd == "RELAY=0" || cmd == "DISABLE")
        {
            _relay.disable();
            send("OK RELAY=OFF");
            return;
        }

        if (cmd.startsWith("F1="))
        {
            int value = cmd.substring(3).toInt();
            _fan1.writeDuty(value);
            send("OK " + statusString());
            return;
        }

        if (cmd.startsWith("F2="))
        {
            int value = cmd.substring(3).toInt();
            _fan2.writeDuty(value);
            send("OK " + statusString());
            return;
        }

        if (cmd.startsWith("ALL="))
        {
            int value = cmd.substring(4).toInt();
            _fan1.writeDuty(value);
            _fan2.writeDuty(value);
            send("OK " + statusString());
            return;
        }

        if (cmd == "STOP")
        {
            _fan1.writeDuty(0);
            _fan2.writeDuty(0);
            send("OK " + statusString());
            return;
        }

        send("ERR Unknown command: " + cmd);
    }

private:
    void handleSerial()
    {
        if (Serial.available())
        {
            String cmd = Serial.readStringUntil('\n');
            processCommand(cmd);
        }
    }

    void handleRpmUpdate()
    {
        uint32_t now = millis();
        if (now - _lastRpmMs >= 1000)
        {
            _fan1.updateRpmEverySecond();
            _fan2.updateRpmEverySecond();
            _lastRpmMs = now;
            //   send(statusString());
        }
    }

    String statusString()
    {
        String s;
        s += "RELAY=";
        s += (_relay.isEnabled() ? "ON" : "OFF");
        s += ",F1=" + String(_fan1.getDuty()) + "%";
        s += ",RPM1=" + String(_fan1.getRpm());
        s += ",F2=" + String(_fan2.getDuty()) + "%";
        s += ",RPM2=" + String(_fan2.getRpm()) + "\n";
        return s;
    }

    void send(const String &msg)
    {
        Serial.println(msg);
        if (_bleConnected && _txCharacteristic != nullptr)
        {
            _txCharacteristic->setValue((msg + "\n").c_str());
            _txCharacteristic->notify();
        }
    }

    void printHelp()
    {
        String h;
        h += "\n=== COMMANDS ===\n";
        h += "ENABLE / RELAY=1   -> relay on\n";
        h += "DISABLE / RELAY=0  -> relay off\n";
        h += "F1=0~100           -> front fan duty\n";
        h += "F2=0~100           -> rear fan duty\n";
        h += "ALL=0~100          -> both fan duty\n";
        h += "STOP               -> both fans off\n";
        h += "STATUS             -> current status\n";
        h += "HELP               -> help\n";
        h += "=================\n";
        send(h);
    }

    // =========================
    // BLE internals
    // =========================
    class ServerCallbacks : public BLEServerCallbacks
    {
    private:
        App *_app;

    public:
        ServerCallbacks(App *app) : _app(app) {}
        void onConnect(BLEServer *pServer) override
        {
            _app->setBleConnected(true);
        }
        void onDisconnect(BLEServer *pServer) override
        {
            _app->setBleConnected(false);
            BLEDevice::startAdvertising();
        }
    };

    class RxCallbacks : public BLECharacteristicCallbacks
    {
    private:
        App *_app;

    public:
        RxCallbacks(App *app) : _app(app) {}
        void onWrite(BLECharacteristic *pCharacteristic) override
        {
            String rx = pCharacteristic->getValue().c_str();
            if (rx.length() > 0)
            {
                _app->processCommand(rx);
            }
        }
    };

    void setupBle()
    {
        BLEDevice::init("AirSplit Veil Fan Controller");

        BLEServer *server = BLEDevice::createServer();
        server->setCallbacks(new ServerCallbacks(this));

        BLEService *service = server->createService(SERVICE_UUID);

        _txCharacteristic = service->createCharacteristic(
            CHARACTERISTIC_UUID_TX,
            BLECharacteristic::PROPERTY_NOTIFY);
        BLECharacteristic *rxCharacteristic = service->createCharacteristic(
            CHARACTERISTIC_UUID_RX,
            BLECharacteristic::PROPERTY_WRITE);
        rxCharacteristic->setCallbacks(new RxCallbacks(this));

        service->start();

        BLEAdvertising *advertising = BLEDevice::getAdvertising();
        advertising->addServiceUUID(SERVICE_UUID);
        advertising->start();
    }
};

// =========================
// Global objects
// =========================
FanController fan1("Front", FAN1_FG_PIN, FAN1_PWM_PIN);
FanController fan2("Rear", FAN2_FG_PIN, FAN2_PWM_PIN);
RelayController relayCtrl(RELAY_PIN);
App app(fan1, fan2, relayCtrl);

// =========================
// ISRs
// =========================
void IRAM_ATTR onFan1Pulse()
{
    if (g_fan1Instance)
        g_fan1Instance->onPulse();
}

void IRAM_ATTR onFan2Pulse()
{
    if (g_fan2Instance)
        g_fan2Instance->onPulse();
}

// =========================
// Arduino entry
// =========================
void setup()
{
    g_fan1Instance = &fan1;
    g_fan2Instance = &fan2;
    app.begin();

    attachInterrupt(digitalPinToInterrupt(FAN1_FG_PIN), onFan1Pulse, FALLING);
    attachInterrupt(digitalPinToInterrupt(FAN2_FG_PIN), onFan2Pulse, FALLING);

    // 開機預設
    relayCtrl.disable();
    fan1.writeDuty(0);
    fan2.writeDuty(0);
}

void loop()
{
    app.loop();
}