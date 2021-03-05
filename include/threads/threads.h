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

enum {
  thrd_success = 0,
  thrd_timedout,
  thrd_busy,
  thrd_nomem,
  thrd_error
};

typedef int (*thrd_start_t)(void *arg);
typedef struct {
  thrd_start_t m_function;
  void *m_arg;
} _thread_start_info;

typedef DWORD thread_storage_key;
struct ThreadData {
  void *value;
  thread_storage_key key;
  struct ThreadData *next;
};

typedef struct mtx_t {
  union {
    CRITICAL_SECTION cs;
    HANDLE mut;
  } m_handle;
  int m_already_locked;
  int m_recursive;
  int m_timed;
} mtx_t;

#define CONDITION_EVENT_ONE 0
#define CONDITION_EVENT_ALL 1
typedef struct cnd_t {
  HANDLE m_events[2];
  unsigned int m_waiters_count;
  CRITICAL_SECTION m_waiters_count_lock;
} cnd_t;

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

#define mtx_plain 0
#define mtx_timed 1
#define mtx_recursive 2

static inline int mtx_init(mtx_t *mtx, int type) {
  mtx->m_already_locked = 0;
  mtx->m_recursive = type & mtx_recursive;
  mtx->m_timed = type & mtx_timed;
  if (!mtx->m_timed) {
    InitializeCriticalSection(&(mtx->m_handle.cs));
  } else {
    mtx->m_handle.mut = CreateMutex(NULL, FALSE, NULL);
    if (mtx->m_handle.mut == NULL) {
      return thrd_error;
    }
  }
  return thrd_success;
}

static inline int mtx_lock(mtx_t *mtx) {
  if (!mtx->m_timed)
    EnterCriticalSection(&(mtx->m_handle.cs));
  else {
    switch (WaitForSingleObject(mtx->m_handle.mut, INFINITE)) {
      case WAIT_OBJECT_0:
        break;
      case WAIT_ABANDONED:
      default:
        return thrd_error;
    }
  }

  if (!mtx->m_recursive) {
    while (mtx->m_already_locked) Sleep(1);
    mtx->m_already_locked = 1;
  }
  return thrd_success;
}

static inline int mtx_unlock(mtx_t *mtx) {
  mtx->m_already_locked = 0;
  if (!mtx->m_timed)
    LeaveCriticalSection(&(mtx->m_handle.cs));
  else {
    if (!ReleaseMutex(mtx->m_handle.mut))
      return thrd_error;
  }
  return thrd_success;
}

static inline int cnd_init(cnd_t *cond) {
  cond->m_waiters_count = 0;

  InitializeCriticalSection(&cond->m_waiters_count_lock);

  cond->m_events[CONDITION_EVENT_ONE] = CreateEvent(NULL, FALSE, FALSE, NULL);
  if (cond->m_events[CONDITION_EVENT_ONE] == NULL) {
    cond->m_events[CONDITION_EVENT_ALL] = NULL;
    return thrd_error;
  }
  cond->m_events[CONDITION_EVENT_ALL] = CreateEvent(NULL, TRUE, FALSE, NULL);
  if (cond->m_events[CONDITION_EVENT_ALL] == NULL) {
    CloseHandle(cond->m_events[CONDITION_EVENT_ONE]);
    cond->m_events[CONDITION_EVENT_ONE] = NULL;
    return thrd_error;
  }

  return thrd_success;
}

static inline int cnd_signal(cnd_t *cond) {
  EnterCriticalSection(&cond->m_waiters_count_lock);
  int have_waiters = (cond->m_waiters_count > 0);
  LeaveCriticalSection(&cond->m_waiters_count_lock);

  if (have_waiters) {
    if (SetEvent(cond->m_events[CONDITION_EVENT_ONE]) == 0)
      return thrd_error;
  }

  return thrd_success;
}

static inline int _cnd_timedwait_win32(cnd_t *cond, mtx_t *mtx, DWORD timeout) {
  EnterCriticalSection(&cond->m_waiters_count_lock);
  ++cond->m_waiters_count;
  LeaveCriticalSection(&cond->m_waiters_count_lock);

  mtx_unlock(mtx);
  DWORD result = WaitForMultipleObjects(2, cond->m_events, FALSE, timeout);
  if (result == WAIT_TIMEOUT) {
    mtx_lock(mtx);
    return thrd_timedout;
  } else if (result == WAIT_FAILED) {
    mtx_lock(mtx);
    return thrd_error;
  }

  EnterCriticalSection(&cond->m_waiters_count_lock);
  --cond->m_waiters_count;
  int last_waiter = (result == (WAIT_OBJECT_0 + CONDITION_EVENT_ALL)) && (cond->m_waiters_count == 0);
  LeaveCriticalSection(&cond->m_waiters_count_lock);

  if (last_waiter) {
    if (ResetEvent(cond->m_events[CONDITION_EVENT_ALL]) == 0) {
      mtx_lock(mtx);
      return thrd_error;
    }
  }

  mtx_lock(mtx);

  return thrd_success;
}

static inline int cnd_wait(cnd_t *cond, mtx_t *mtx) {
  return _cnd_timedwait_win32(cond, mtx, INFINITE);
}

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
