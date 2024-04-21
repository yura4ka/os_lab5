#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <unistd.h>

#define ASKING_INTERVAL 5

typedef struct {
  int x;
  int fReady;
  int gReady;
  int fResult;
  int gResult;
} SharedData;

int f(int x) {
  if (x < 0) {
    while (1) {
      x += 0;
    }
  }

  if (x > 10) {
    sleep(x);
    return x * x - 256;
  }

  return -x * 5;
}

int g(int x) {
  if (x < 0 || x > 10) {
    return -x * 5;
  }

  sleep(x * 3);
  return x + 12;
}

void* calculateF(void* arg) {
  int shmid = *((int*)arg);
  SharedData* data = (SharedData*)shmat(shmid, NULL, 0);
  data->fResult = f(data->x);
  data->fReady = 1;
  shmdt(data);
  return NULL;
}
void* calculateG(void* arg) {
  int shmid = *((int*)arg);
  SharedData* data = (SharedData*)shmat(shmid, NULL, 0);
  data->gResult = g(data->x);
  data->gReady = 1;
  shmdt(data);
  return NULL;
}

int main() {
  int shmid = shmget(IPC_PRIVATE, 1024, IPC_CREAT | 0666);
  SharedData* data = (SharedData*)shmat(shmid, NULL, 0);

  printf("Enter x: ");
  scanf("%d", &(data->x));
  data->fReady = data->gReady = 0;

  pthread_t thread1, thread2;
  pthread_create(&thread1, NULL, calculateF, &shmid);
  pthread_create(&thread2, NULL, calculateG, &shmid);

  pthread_detach(thread1);
  pthread_detach(thread2);

  time_t lastPrint = time(NULL);
  int askUser = 1;
  int stopExecution = 0;
  int result;

  while (!stopExecution) {
    if (data->fReady && data->gReady) {
      result = data->fResult * data->gResult;
      break;
    } else if (data->fReady && data->fResult == 0 ||
               data->gReady && data->gResult == 0) {
      result = 0;
      break;
    }

    if (askUser && difftime(time(NULL), lastPrint) >= ASKING_INTERVAL) {
      char action;
      printf(
          "Looks like calculations take quite a while! Do you want to stop "
          "the "
          "program? y - yes, n - no, d - no, don't ask again\n");
      scanf(" %c", &action);
      lastPrint = time(NULL);
      if (action == 'y') {
        stopExecution = 1;
        break;
      };
      if (action == 'd') askUser = 0;
    }
  }

  pthread_cancel(thread1);
  pthread_cancel(thread2);

  shmdt(data);
  shmctl(shmid, IPC_RMID, NULL);

  if (stopExecution) {
    printf("Execution stopped\n");
  } else {
    printf("Result: %d\n", result);
  }
  return 0;
}
