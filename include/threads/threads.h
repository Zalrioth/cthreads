/*#pragma once
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

enum {
  thrd_success = 0,
  thrd_timedout,
  thrd_busy,
  thrd_nomem,
  thrd_error
};

// TODOOOOOOOOOOOOOOOOOOOO: Support pthread

int thrd_create(thrd_t *thr, thrd_start_t func, void *arg) {
  *thr = CreateThread(NULL, 0, func, arg, 0, NULL);

  return thrd_success;
}
*/