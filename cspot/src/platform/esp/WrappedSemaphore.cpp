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
    xSemaphoreTake(semaphoreHandle, portMAX_DELAY);
    return 0;
}

void WrappedSemaphore::give()
{

    xSemaphoreGive(semaphoreHandle);
}
