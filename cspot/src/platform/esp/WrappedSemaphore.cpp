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

void WrappedSemaphore::wait()
{
    xSemaphoreTake(semaphoreHandle, portMAX_DELAY);
}

void WrappedSemaphore::give()
{

    xSemaphoreGive(semaphoreHandle);
}
