#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include "tpool.h"

void waitForJob(TPool* pool)
{
  pthread_mutex_lock(&pool->jobsMutex);
  while(pool->condValue != 1){
    pthread_cond_wait(&pool->jobsCond, &pool->jobsMutex);
  }
  pthread_mutex_unlock(&pool->jobsMutex);
}

void broadcastJob(TPool* pool)
{
  pthread_mutex_lock(&pool->jobsMutex);
  pool->condValue = 1;
  pthread_cond_broadcast(&pool->jobsCond);
  pthread_mutex_unlock(&pool->jobsMutex);
}

void signalJob(TPool* pool)
{
  pthread_mutex_lock(&pool->jobsMutex);
  pool->condValue = 1;
  pthread_cond_signal(&pool->jobsCond);
  pthread_mutex_unlock(&pool->jobsMutex);
}

void tpoolEnqueue(TPool* pool, jobFunc func, void* arg)
{
  if (pool->joining)
  {
    return;
  }

  Queue* queue = pool->queue;

  pthread_mutex_lock(&queue->mutex);

#if TPOOL_VERBOSE
  printf("Enqueuing a job. Current length is %d\n", queue->length);
#endif

  Job* job = malloc(sizeof(Job));
  job->func = func;
  job->arg = arg;

  switch (queue->length)
  {
    case 0:
      {
        queue->first = job;
        queue->last = job;
      } break;
    default:
      {
        queue->last->nextJob = job;
        queue->last = job;
      }
  }
  queue->length++;

  signalJob(pool);

  pthread_mutex_unlock(&queue->mutex);
}

Job* queuePoll(TPool* pool)
{
  Queue* queue = pool->queue;

  pthread_mutex_lock(&queue->mutex);

  Job* job = queue->first;

  switch (queue->length)
  {
    case 0:
      {
        // pass
      } break;
    case 1:
      {
        queue->first = NULL;
        queue->last = NULL;
        queue->length = 0;
      } break;
    default:
      {
        queue->first = queue->first->nextJob;
        queue->length--;

        signalJob(pool);
      }
  }

  pthread_mutex_unlock(&queue->mutex);

  return job;
}

void* infiniteWork(void* arg)
{
  Thread* thread = (Thread*)arg;
  TPool* pool = thread->pool;
  assert(pool);

  // wait for all threads
  pthread_barrier_wait(&pool->startBarrier);

  while (pool->working)
  {
    waitForJob(pool);

    if (pool->working) {
      Job* job = queuePoll(pool);

      if (job == NULL)
      {
        if (pool->joining)
        {
          break;
        }

        continue;
      }

#if TPOOL_VERBOSE
  printf("Thread %d picked up a new job\n", thread->id);
#endif

      job->func(job->arg);
      free(job);
    }
  }

  // if joining, wait for all threads
  if (pool->joining) {
    pthread_barrier_wait(&pool->joinBarrier);
  }

  return 0;
}

TPool* tpoolCreate(int threadsCount)
{
#if TPOOL_VERBOSE
  printf("TPOOL: Creating a pool. Number of threads: %d\n", threadsCount);
#endif

  TPool* pool = (TPool*)malloc(sizeof(TPool));
  if (pool == NULL)
  {
    return 0;
  }

  pool->threadsCount = threadsCount;
  pool->working = true;
  pool->joining = false;
  pool->condValue = 0;

  int res = pthread_barrier_init(&pool->startBarrier, 0, pool->threadsCount);
  if (res) {
    free(pool);
    return NULL;
  }

  res = pthread_barrier_init(&pool->joinBarrier, 0, pool->threadsCount + 1); // account for main thread
  if (res) {
    free(pool);
    return NULL;
  }

  res = pthread_mutex_init(&pool->jobsMutex, 0);
  if (res) {
    free(pool);
    return NULL;
  }

  res = pthread_cond_init(&pool->jobsCond, 0);
  if (res) {
    free(pool);
    return NULL;
  }

  pool->threads = (Thread*)malloc(sizeof(Thread) * pool->threadsCount);
  if (pool->threads == NULL)
  {
    free(pool);
    return NULL;
  }

  pool->queue = (Queue*)malloc(sizeof(Queue));
  if (pool->queue == NULL) {
    free(pool);

    return NULL;
  }

  pool->queue->first = NULL;
  pool->queue->last = NULL;
  pool->queue->length = 0;

  res = pthread_mutex_init(&pool->queue->mutex, 0);
  if (res) {
    free(pool->queue);
    free(pool);

    return NULL;
  }

  for (int i = 0; i < pool->threadsCount; i++)
  {
    // prepare bookkeeping
    Thread* thread = &pool->threads[i];

    thread->pool = pool;
    thread->id = i;

    // start thread
    pthread_create(&thread->pthread, 0, infiniteWork, thread);
    pthread_detach(thread->pthread);
  }

#if TPOOL_VERBOSE
  printf("TPOOL: Finished initization\n");
#endif

  return pool;
}

void tpoolJoin(TPool* pool)
{
#if TPOOL_VERBOSE
  printf("TPOOL: Joining\n");
#endif

  pool->joining = true;

  broadcastJob(pool);

  pthread_barrier_wait(&pool->joinBarrier);

#if TPOOL_VERBOSE
  printf("TPOOL: Joined\n");
#endif

  return;
}
