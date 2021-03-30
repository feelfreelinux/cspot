#include "platform/WrappedSemaphore.h"

WrappedSemaphore::WrappedSemaphore(int count)
{
    sem_init(&this->semaphoreHandle, 0, 0); // eek pointer
}

WrappedSemaphore::~WrappedSemaphore()
{
    sem_destroy(&this->semaphoreHandle);
}

void WrappedSemaphore::wait()
{
    sem_wait(&this->semaphoreHandle);
}

void WrappedSemaphore::give()
{
    sem_post(&this->semaphoreHandle);
}
