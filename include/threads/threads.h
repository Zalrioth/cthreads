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

#if !(defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201102L)) && !defined(_Thread_local)
#if defined(__GNUC__) || defined(__INTEL_COMPILER) || defined(__SUNPRO_CC) || defined(__IBMCPP__)
#define _Thread_local __thread
#else
#define _Thread_local __declspec(thread)
#endif
#elif defined(__GNUC__) && defined(__GNUC_MINOR__) && (((__GNUC__ << 8) | __GNUC_MINOR__) < ((4 << 8) | 9))
#define _Thread_local __thread
#endif

#if defined(THREAD_WIN32)
typedef HANDLE thrd_t;
#else
typedef pthread_t thrd_t;
#endif

typedef int (*thrd_start_t)(void *arg);
typedef struct {
  thrd_start_t m_function;
  void *m_arg;
} _thread_start_info;

typedef DWORD tss_t;
struct ThreadData {
  void *value;
  tss_t key;
  struct ThreadData *next;
};

static _Thread_local struct ThreadData *thread_head = NULL;
static _Thread_local struct ThreadData *thread_tail = NULL;
typedef void (*dtor_t)(void *val);
static dtor_t thread_dtors[1088] = {
    NULL,
};
#define DTOR_ITERATIONS (4)
static void thread_cleanup(void) {
  struct ThreadData *data;
  int iteration;
  unsigned int again = 1;
  void *value;

  for (iteration = 0; iteration < DTOR_ITERATIONS && again > 0; iteration++) {
    again = 0;
    for (data = thread_head; data != NULL; data = data->next) {
      if (data->value != NULL) {
        value = data->value;
        data->value = NULL;

        if (thread_dtors[data->key] != NULL) {
          again = 1;
          thread_dtors[data->key](value);
        }
      }
    }
  }

  while (thread_head != NULL) {
    data = thread_head->next;
    free(thread_head);
    thread_head = data;
  }
  thread_head = NULL;
  thread_tail = NULL;
}

static DWORD WINAPI _thrd_wrapper_function(LPVOID aArg) {
  thrd_start_t fun;
  void *arg;
  int res;

  _thread_start_info *ti = (_thread_start_info *)aArg;
  fun = ti->m_function;
  arg = ti->m_arg;

  free((void *)ti);

  res = fun(arg);

  if (thread_head != NULL) {
    thread_cleanup();
  }

  return (DWORD)res;
}

enum {
  thrd_success = 0,
  thrd_timedout,
  thrd_busy,
  thrd_nomem,
  thrd_error
};

static inline int thrd_create(thrd_t *thr, thrd_start_t func, void *arg) {
  _thread_start_info *ti = (_thread_start_info *)malloc(sizeof(_thread_start_info));
  if (ti == NULL)
    return thrd_nomem;

  ti->m_function = func;
  ti->m_arg = arg;

  *thr = CreateThread(NULL, 0, _thrd_wrapper_function, (LPVOID)ti, 0, NULL);
  if (!*thr) {
    free(ti);
    return thrd_error;
  }

  return thrd_success;
}

static inline int thrd_join(thrd_t thr, int *res) {
  DWORD dwRes;

  if (WaitForSingleObject(thr, INFINITE) == WAIT_FAILED) {
    return thrd_error;
  }
  if (res != NULL) {
    if (GetExitCodeThread(thr, &dwRes) != 0) {
      *res = (int)dwRes;
    } else {
      return thrd_error;
    }
  }
  CloseHandle(thr);

  return thrd_success;
}

static inline int thrd_detach(thrd_t thr) {
  return CloseHandle(thr) != 0 ? thrd_success : thrd_error;
}

#endif  // THREADS_H
