#ifndef WRAPPEDSEMAPHORE_H
#define WRAPPEDSEMAPHORE_H


#ifdef ESP_PLATFORM
#include "freertos/freertos.h"
#include "freertos/semphr.h"
#else
#include <dispatch/dispatch.h>
#endif

class WrappedSemaphore {
private:
#ifdef ESP_PLATFORM
    xSemaphoreHandle semaphoreHandle;
#else
    dispatch_semaphore_t semaphoreHandle;
#endif
public:
    WrappedSemaphore(int maxVal = 200);
    ~WrappedSemaphore();

    void wait();
    void give();
};

#endif