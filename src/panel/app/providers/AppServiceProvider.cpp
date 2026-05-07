#include "app/providers/AppServiceProvider.h"

AppServiceProvider::AppServiceProvider(PanelApplicationContext &context) : context_(context) {
}

void AppServiceProvider::registerServices() {
    Serial.println("Create application");
    context_.app = new AppController(context_.panel, context_.registry, *context_.network, kSelfRole, kSelfId);
    context_.meshBridge.setApplication(context_.app);
}

void AppServiceProvider::boot() {
    if (context_.app != nullptr) {
        context_.app->begin();
    }
}
