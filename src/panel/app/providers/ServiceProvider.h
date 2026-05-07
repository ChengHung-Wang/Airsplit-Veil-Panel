#pragma once

class ServiceProvider {
public:
    virtual ~ServiceProvider() = default;

    virtual void registerServices() = 0;

    virtual void boot() = 0;
};
