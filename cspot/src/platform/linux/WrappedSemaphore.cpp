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

    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
    {
        /* handle error */
        return -1;
    }

    ts.tv_usec += 1000;
    while ((s = sem_timedwait(&this->semaphoreHandle, &ts)) == -1 && errno == EINTR)
        continue;       /* Restart if interrupted by handler */
    if (s == -1)
    {
        return 1;
    }
    else
        return 0;
}

void WrappedSemaphore::give()
{
    sem_post(&this->semaphoreHandle);
}
