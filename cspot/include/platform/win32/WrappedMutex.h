#ifndef WIN32_WRAPPED_MUTEX_H
#define WIN32_WRAPPED_MUTEX_H
#include <Windows.h>

/**
 * Wraps a windows mutex.
 * Header only since all the methods call one function, we want them to be inlined
 **/
class WrappedMutex
{
public:
    WrappedMutex()
    {
        this->_mutex = CreateMutex(
            NULL,  // default security attributes
            FALSE, // initially not owned
            NULL); // unnamed mutex
    }

    ~WrappedMutex()
    {
        CloseHandle(_mutex);
    }

    void lock()
    {
        WaitForSingleObject(_mutex, INFINITE);
    }

    void unlock()
    {
        ReleaseMutex(_mutex);
    }

private:
    HANDLE _mutex;
};

#endif
