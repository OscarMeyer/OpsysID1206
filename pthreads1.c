#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

int loop = 0;

int count = 0;

void *hello(void *name) {

  for(int i = 0; i < loop; i++) {
    //printf("hello %s %d\n", (char *)name, count);
    count++;
    //sleep(2);
  }
}

int main(int argc, char *argv[]) {

  if(argc < 2) {
    printf("usage: count <loop>\n");
    exit(1);
  }

  loop = atoi(argv[1]);

  pthread_t p1, p2;

  pthread_create(&p1, NULL, hello, "A");
  //sleep(1);
  pthread_create(&p2, NULL, hello, "B");
  pthread_join(p1, NULL);
  pthread_join(p2, NULL);

  printf("Mamma: result is %d\n", count);
  return 0;
}
