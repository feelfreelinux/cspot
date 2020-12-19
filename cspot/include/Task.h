#ifndef TASK_H
#define TASK_H

#ifdef ESP_PLATFORM
#include <pthread.h>
#include <esp_pthread.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#elif _WIN32
#include <Windows.h>
#else
#include <pthread.h>
#endif

class Task
{
public:
   Task() {}
   virtual ~Task() {}

#ifdef ESP_PLATFORM
   bool startTask()
   {
      esp_pthread_cfg_t cfg = esp_pthread_get_default_config();
      cfg.stack_size = (8 * 1024);
      cfg.inherit_cfg = true;
      cfg.pin_to_core = 1;
      esp_pthread_set_cfg(&cfg);
      return (pthread_create(&_thread, NULL, taskEntryFunc, this) == 0);
   }
#elif _WIN32
   bool startTask()
   {
      this->_thread = CreateThread(
          NULL, // default security attributes
          0,
          (LPTHREAD_START_ROUTINE)taskEntryFunc,
          this,
          0, // default creation flags
          NULL);

      return true;
   }
#else
   bool startTask()
   {
      return (pthread_create(&_thread, NULL, taskEntryFunc, this) == 0);
   }
#endif
#ifdef _WIN32
   void waitForTaskToReturn()
   {
      WaitForSingleObject(_thread, 0L);
   }
#else
   void waitForTaskToReturn()
   {
      (void)pthread_join(_thread, NULL);
   }
#endif

protected:
   virtual void runTask() = 0;

private:
   static void *taskEntryFunc(void *This)
   {
      ((Task *)This)->runTask();
      return NULL;
   }

#ifdef _WIN32
   HANDLE _thread;
#else
   pthread_t _thread;
#endif
};

#endif
