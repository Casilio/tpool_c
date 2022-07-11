#include <unistd.h>
#include <stdio.h>
#include "tpool.h"

#include <stdlib.h>

// instant job
void* firstJob(void* arg)
{
  int num = *(int*)arg;

  printf("Simulating a work with number %d\n", num);
  free(arg);

  return 0;
}

// job with some delay
void* secondJob(void* arg)
{
  int num = *(int*)arg;

  printf("Simulating a long work with number %d\n", num);
  sleep(1);
  free(arg);

  return 0;
}

int main()
{
  TPool* pool = tpoolCreate(5);

  // low load. single element queue
  for (int i = 0; i < 5; i++) {
    int* num = (int*)malloc(sizeof(int));
    *num = i + 10 * 25 / 7;

    tpoolEnqueue(pool, firstJob, num);
    sleep(1);
  };

  sleep(1);

  printf("Spamming jobs\n");

  // high load. long queue
  for (int i = 0; i < 20; i++) {
    int* num = malloc(sizeof(int));
    *num = i + 10 * 25 / 7;

    tpoolEnqueue(pool, secondJob, num);
  };

  printf("Finishing main\n");
  tpoolJoin(pool);
  tpoolDestroy(pool);

  printf("Finished main\n");
  return 0;
}
