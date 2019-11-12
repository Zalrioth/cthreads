#pragma once
#ifndef THREADS_H
#define THREADS_H

#if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)
#define THREAD_WIN32
#else
#define THREAD_POSIX
#endif

#include <time.h>

#ifdef THREAD_WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#define __UNDEF_LEAN_AND_MEAN
#endif
#include <windows.h>
#ifdef __UNDEF_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#undef __UNDEF_LEAN_AND_MEAN
#endif
#else
#include <pthread.h>
#endif

#if defined(THREAD_WIN32)
typedef HANDLE thrd_t;
#else
typedef pthread_t thrd_t;
#endif

#define THREAD_FAILED 0
#define THREAD_NO_ERROR 1
#define THREAD_TIMEOUT 2
#define THREAD_BUSY 3
#define THREAD_NO_MEMORY 4

struct ThreadStartInfo {
  int (*func)(void *);
  void *arg;
};

#if defined(THREAD_WIN32)
static DWORD WINAPI thrd_wrapper_function(LPVOID arg)
#else
static void *thrd_wrapper_function(void *arg)
#endif
{
  struct ThreadStartInfo *start_info = (struct ThreadStartInfo *)arg;
  int result = start_info->func(start_info->arg);

  free((void *)start_info);

#if defined(THREAD_WIN32)
  if (_tinycthread_tss_head != NULL) {
    _tinycthread_tss_cleanup();
  }

  return (DWORD)result;
#else
  return (void *)(intptr_t)result;
#endif
}

int thrd_create(thrd_t *thread, void *entry_point, void *arg) {
  struct ThreadStartInfo *start_info = (struct ThreadStartInfo *)malloc(sizeof(struct ThreadStartInfo));
  if (start_info == NULL)
    return THREAD_NO_MEMORY;

#if defined(THREAD_WIN32)
  typedef HANDLE thrd_t;
#else
  typedef pthread_t thrd_t;
#endif

  return THREAD_NO_ERROR;
}
