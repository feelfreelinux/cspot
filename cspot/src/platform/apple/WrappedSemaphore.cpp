#include "platform/WrappedSemaphore.h"

WrappedSemaphore::WrappedSemaphore(int count)
{
    semaphoreHandle = dispatch_semaphore_create(0);
}

WrappedSemaphore::~WrappedSemaphore()
{
    dispatch_release(semaphoreHandle);
}

int WrappedSemaphore::wait()
{
    dispatch_time_t timeout = dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC / 10);

    return dispatch_semaphore_wait(semaphoreHandle, timeout);
}

void WrappedSemaphore::give()
{
    dispatch_semaphore_signal(semaphoreHandle);
}
