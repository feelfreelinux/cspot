#include "WrappedSemaphore.h"

WrappedSemaphore::WrappedSemaphore(int count) {
    #ifdef ESP_PLATFORM
    semaphoreHandle = xSemaphoreCreateCounting(count, 0);
    #else
    semaphoreHandle = dispatch_semaphore_create(0);
    #endif
}

WrappedSemaphore::~WrappedSemaphore() {
    #ifdef ESP_PLATFORM
    vSemaphoreDelete(semaphoreHandle);
    #else
    dispatch_release(semaphoreHandle);
    #endif
}


void WrappedSemaphore::wait() {
    #ifdef ESP_PLATFORM
    xSemaphoreTake(semaphoreHandle, portMAX_DELAY);
    #else
    dispatch_semaphore_wait(semaphoreHandle, DISPATCH_TIME_FOREVER);
    #endif
}

void WrappedSemaphore::give() {
    #ifdef ESP_PLATFORM
    xSemaphoreGive(semaphoreHandle);
    #else
    dispatch_semaphore_signal(semaphoreHandle);
    #endif
}