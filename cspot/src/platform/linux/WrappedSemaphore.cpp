#include "platform/WrappedSemaphore.h"

WrappedSemaphore::WrappedSemaphore(int count)
{
    sem_init(&this->semaphoreHandle, 0, 0); // eek pointer
}

WrappedSemaphore::~WrappedSemaphore()
{
    sem_destroy(&this->semaphoreHandle);
}

int WrappedSemaphore::wait()
{
    sem_wait(&this->semaphoreHandle);
    return 0;
}

void WrappedSemaphore::give()
{
    sem_post(&this->semaphoreHandle);
}
