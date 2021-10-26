#include "platform/WrappedSemaphore.h"

/**
 * Platform semaphopre implementation for the esp-idf.
 */

WrappedSemaphore::WrappedSemaphore(int count)
{
    semaphoreHandle = xSemaphoreCreateCounting(count, 0);
}

WrappedSemaphore::~WrappedSemaphore()
{
    vSemaphoreDelete(semaphoreHandle);
}

int WrappedSemaphore::wait()
{
    if (xSemaphoreTake(semaphoreHandle, 1000) == pdTRUE) {
        return 0;
    }

    return 1;
}

void WrappedSemaphore::give()
{

    xSemaphoreGive(semaphoreHandle);
}
