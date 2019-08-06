#pragma once
#ifndef THREADS_H
#define THREADS_H

#ifdef _WIN32
#include <windows.h>

struct Thread
{
    HANDLE handle;
    //HANDLE mutex;
};

//https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-resumethread
//https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-suspendthread
//https://docs.microsoft.com/en-us/windows/win32/sync/using-mutex-objects
//https://stackoverflow.com/questions/1981459/using-threads-in-c-on-windows-simple-example
//https://docs.microsoft.com/en-us/windows/win32/procthread/creating-threads
int thread_create(struct Thread *thread, void *entryPoint, void *data)
{
    //thread->mutex = CreateMutex(NULL, FALSE, NULL);
    thread->handle = CreateThread(NULL, 0, entryPoint, data, 0, NULL);
    // Error starting thread
    if (!thread->handle)
        return 1;

    //https://stackoverflow.com/questions/418742/is-it-reasonable-to-call-closehandle-on-a-thread-before-it-terminates#418748
    //Mark thread for closing to let thread die when finished executing
    CloseHandle(thread->handle);
    //CloseHandle(thread->mutex);

    return 0;
}
void thread_start(struct Thread *thread)
{
    ResumeThread(thread->handle);
}
void thread_stop(struct Thread *thread)
{
    SuspendThread(thread->handle);
}

#else
#include <pthread.h>
#include <pthread_np.h>

struct Thread
{
    pthread_t handle;
    //int runFlag;
    //pthread_mutex_t mutex;
    //pthread_cond_t condition;
};

//https://man.openbsd.org/OpenBSD-5.1/pthread_resume_np.3
//https://stackoverflow.com/questions/11468333/linux-threads-suspend-resume
//https://www.geeksforgeeks.org/multithreading-c-2/
//https://www.thegeekstuff.com/2012/04/create-threads-in-linux/
//https://randu.org/tutorials/threads/
//https://stackoverflow.com/questions/13662689/what-is-the-best-solution-to-pause-and-resume-pthreads
int thread_create(struct Thread *thread, void *entryPoint, void *data)
{
    //pthread_mutex_init(&(thread_args->mutex), NULL);
    //pthread_cond_init(&(thread_args->condition), NULL);
    pthread_create(&thread->handle, NULL, entryPoint, data);

    if (!thread->handle)
        return 1;
}
void thread_start(struct Thread *thread)
{
    pthread_resume_np(thread->handle);
    // Unlocks mutex
    //pthread_mutex_lock(&(thread->mutex));
    //thread->runFlag = 0;
    //phtread_cond_broadcast(&(thread->condition));
    //pthread_mutex_unlock(&(thread->mutex));
}
void thread_stop(struct Thread *thread)
{
    pthread_suspend_np(thread->handle);
    // Locks current thread with mutex
    //pthread_mutex_lock(&(thread->mutex));
    //thread->runFlag = 1;
    //pthread_mutex_unlock(&(thread->mutex));
}

#endif

#endif
