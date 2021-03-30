#include "platform/WrappedSemaphore.h"

WrappedSemaphore::WrappedSemaphore(int count)
{
    semaphoreHandle = dispatch_semaphore_create(0);
}

WrappedSemaphore::~WrappedSemaphore()
{
    dispatch_release(semaphoreHandle);
}

void WrappedSemaphore::wait()
{
    dispatch_semaphore_wait(semaphoreHandle, DISPATCH_TIME_FOREVER);
}

void WrappedSemaphore::give()
{
    dispatch_semaphore_signal(semaphoreHandle);
}
