#ifndef WRAPPEDSEMAPHORE_H
#define WRAPPEDSEMAPHORE_H


#ifdef ESP_PLATFORM
#include "freertos/freertos.h"
#include "freertos/semphr.h"
#elif __APPLE__
#include <dispatch/dispatch.h>
#else

#endif

class WrappedSemaphore {
private:
#ifdef ESP_PLATFORM
    xSemaphoreHandle semaphoreHandle;
#elif __APPLE__
    dispatch_semaphore_t semaphoreHandle;
#else
#endif
public:
    WrappedSemaphore(int maxVal = 200);
    ~WrappedSemaphore();

    void wait();
    void give();
};

#endif