#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>


volatile int count;

pthread_mutex_t global;


int fib(int i) {
  if( i == 0 )
    return 0;
  if (i == 1)
    return 1;
  return fib(i-1) + fib(i-2);
}


typedef struct {int inc; int id; pthread_mutex_t *mutex;} args;

void *increment(void *arg) {

  int inc = ((args*)arg)->inc;
  int id = ((args*)arg)->id;
  pthread_mutex_t *mutex = ((args*)arg)->mutex;

  printf("start %d\n", id);

  for(int i = 0; i < inc; i++) {
    pthread_mutex_lock(mutex);
    count++;
    pthread_mutex_unlock(mutex);
  }
  printf("stop %d\n", id);
}


int main(int argc, char *argv[]) {

  if(argc != 2) {
    printf("usage mutex <inc>\n");
    exit(0);
  }

  int inc = atoi(argv[1]);

  pthread_t p1, p2;

  args a1, a2;

  pthread_mutex_init(&global, NULL);

  a1.inc = inc;
  a1.inc = inc;

  a1.id = 1;
  a2.id = 2;

  a1.mutex = &global;
  a2.mutex = &global;

  pthread_create(&p1, NULL, increment, &a1);
  pthread_create(&p2, NULL, increment, &a1);
  pthread_join(p1, NULL);
  pthread_join(p2, NULL);

  printf("result is %d\n", count);

  return 0;
}
