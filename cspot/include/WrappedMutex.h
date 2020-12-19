#ifndef MUTEX_H
#define MUTEX_H
#ifdef _WIN32
#include <Windows.h>
#include <iostream> // error print
#else
#include <pthread.h>
#endif

/**
 * Wraps a pthread mutex or a windows mutex.
 * Header only since all the methods call one function, we want them to be inlined
 **/
class WrappedMutex
{
public:
#ifdef _WIN32
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

#else
    WrappedMutex()
    {
        pthread_mutex_init(&this->_mutex, NULL);
    }

    ~WrappedMutex()
    {
        pthread_mutex_destroy(&this->_mutex);
    }

    void lock()
    {
        pthread_mutex_lock(&this->_mutex);
    }

    void unlock()
    {
        pthread_mutex_unlock(&this->_mutex);
    }

#endif
private:
#ifdef _WIN32
    HANDLE _mutex;
#else
    pthread_mutex_t _mutex;
#endif
};

#endif
