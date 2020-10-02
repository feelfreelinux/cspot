#include "WrappedSemaphore.h"



#ifdef ESP_PLATFORM
WrappedSemaphore::WrappedSemaphore(int count) {
    semaphoreHandle = xSemaphoreCreateCounting(count, 0);
}

WrappedSemaphore::~WrappedSemaphore() {
    vSemaphoreDelete(semaphoreHandle);
}


void WrappedSemaphore::wait() {
    xSemaphoreTake(semaphoreHandle, portMAX_DELAY);
}

void WrappedSemaphore::give() {
   
    xSemaphoreGive(semaphoreHandle);
}
#elif __APPLE__
   WrappedSemaphore::WrappedSemaphore(int count) {
    semaphoreHandle = dispatch_semaphore_create(0);
}

WrappedSemaphore::~WrappedSemaphore() {
   dispatch_release(semaphoreHandle);
}


void WrappedSemaphore::wait() {
   dispatch_semaphore_wait(semaphoreHandle, DISPATCH_TIME_FOREVER);
}

void WrappedSemaphore::give() {
    dispatch_semaphore_signal(semaphoreHandle);
}
#else

WrappedSemaphore::WrappedSemaphore(int count) {

}

WrappedSemaphore::~WrappedSemaphore() {

}


void WrappedSemaphore::wait() {

}

void WrappedSemaphore::give() {

}

#endif

