#include "WrappedSemaphore.h"

#ifdef ESP_PLATFORM
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
#elif __APPLE__
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
#elif _WIN32
WrappedSemaphore::WrappedSemaphore(int count)
{
    semaphoreHandle = CreateSemaphore(
        NULL,           // default security attributes
        0,  // initial count
        count,  // maximum count
        NULL);          // unnamed semaphore

}

WrappedSemaphore::~WrappedSemaphore()
{
    CloseHandle(semaphoreHandle);
}

void WrappedSemaphore::wait()
{
    WaitForSingleObject(semaphoreHandle, INFINITE);
}

void WrappedSemaphore::give()
{
    ReleaseSemaphore(semaphoreHandle, 1, NULL);
}
#else

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

#endif
