#ifndef UNIXLIKE_WRAPPED_MUTEX_H
#define UNIXLIKE_WRAPPED_MUTEX_H
#include <pthread.h>

/**
 * Wraps a mutex on unixlike patforms (linux, macOS, esp32 etc.).
 * Header only since all the methods call one function, we want them to be inlined
 **/
class WrappedMutex
{
public:
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

private:
    pthread_mutex_t _mutex;
};

#endif
