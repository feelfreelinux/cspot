#ifndef TASK_H
#define TASK_H

#include <pthread.h>

class Task
{
public:
   Task() {}
   virtual ~Task() {}

   bool startTask()
   {
      return (pthread_create(&_thread, NULL, taskEntryFunc, this) == 0);
   }
   void waitForTaskToReturn()
   {
      (void) pthread_join(_thread, NULL);
   }

protected:
   virtual void runTask() = 0;

private:
   static void * taskEntryFunc(void * This) {((Task *)This)->runTask(); return NULL;}

   pthread_t _thread;
};

#endif