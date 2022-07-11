#ifndef __TPOOL_H__
#define __TPOOL_H__

#include <pthread.h>
#include <stdbool.h>

typedef struct TPool TPool;

typedef struct {
  int id;
  pthread_t pthread;
  TPool* pool;
} Thread;

typedef void* (*jobFunc)(void* arg);

typedef struct Job {
  jobFunc func;
  void* arg;
  struct Job* nextJob;
} Job;

typedef struct {
  pthread_mutex_t mutex;
  Job* first;
  Job* last;
  int length;
} Queue;

// public for now
typedef struct TPool {
  Thread* threads;
  int threadsCount;

  Queue *queue;

  bool working;
  bool joining;

  pthread_barrier_t joinBarrier;
  pthread_barrier_t startBarrier;

  pthread_mutex_t jobsMutex;
  pthread_cond_t jobsCond;
  int condValue;
} TPool;

TPool* tpoolCreate(int threadsCount);
void tpoolJoin(TPool* pool);
void tpoolEnqueue(TPool* pool, jobFunc func, void* arg);
void tpoolDestroy(TPool* pool);

#endif
