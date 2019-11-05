#include "../include/threads/threads.h"

void *testFunction(void *data) {
  printf("Hello from thread!\n");

  return NULL;
}

int main(int argc, char *argv[]) {
  struct Thread thread;

  int threadError = thread_create(&thread, testFunction, NULL);
  if (threadError == 1)
    printf("Error starting thread!\n");

  return 0;
}