#ifndef TASK_H
#define TASK_H


#ifdef ESP_PLATFORM
#include <esp_pthread.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#endif

#include <pthread.h>

class Task
{
public:
   Task() {}
   virtual ~Task() {}

   bool startTask()
   {
#ifdef ESP_PLATFORM
      esp_pthread_cfg_t cfg = esp_pthread_get_default_config();
      cfg.stack_size = (12 * 1024);
      cfg.inherit_cfg = true;
      cfg.pin_to_core = tskNO_AFFINITY;
      esp_pthread_set_cfg(&cfg);
#endif
      return (pthread_create(&_thread, NULL, taskEntryFunc, this) == 0);
   }
   void waitForTaskToReturn()
   {
      (void)pthread_join(_thread, NULL);
   }

protected:
   virtual void runTask() = 0;

private:
   static void *taskEntryFunc(void *This)
   {
      ((Task *)This)->runTask();
      return NULL;
   }

   pthread_t _thread;
};

#endif