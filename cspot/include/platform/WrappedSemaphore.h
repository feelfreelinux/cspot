#ifndef WRAPPEDSEMAPHORE_H
#define WRAPPEDSEMAPHORE_H

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#elif __APPLE__
#include <dispatch/dispatch.h>
#else
#include <semaphore.h>
#endif

class WrappedSemaphore
{
private:
#ifdef ESP_PLATFORM
    xSemaphoreHandle semaphoreHandle;
#elif __APPLE__
    dispatch_semaphore_t semaphoreHandle;
#else
    sem_t semaphoreHandle;
#endif
public:
    WrappedSemaphore(int maxVal = 200);
    ~WrappedSemaphore();

    int wait();
    void give();
};

#endif