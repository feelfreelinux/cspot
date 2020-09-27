#include "WrappedSemaphore.h"

WrappedSemaphore::WrappedSemaphore(int count) {
    #ifdef ESP_PLATFORM
    semaphoreHandle = xSemaphoreCreateCounting(count, 0);
    #endif
}

WrappedSemaphore::~WrappedSemaphore() {
    #ifdef ESP_PLATFORM
    vSemaphoreDelete(semaphoreHandle);
    #endif
}


void WrappedSemaphore::wait() {
    #ifdef ESP_PLATFORM
    xSemaphoreTake(semaphoreHandle, portMAX_DELAY);
    #endif
}

void WrappedSemaphore::give() {
    #ifdef ESP_PLATFORM
    xSemaphoreGive(semaphoreHandle);
    #endif
}