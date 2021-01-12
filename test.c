#include <stdio.h>
#include "green.h"
#include <unistd.h>

int numThreads = 2;
int global = 0;
int counter = 0;
int flag = 0;
green_mutex_t mutex;
green_cond_t cond, full;
//Write doesnt always write what we want it to write ;( ) GIves stacksmashing
// void *test(void *arg){
//   int i = *(int*)arg;
//   int loop = 20;
//   while(loop > 0) {
//      // char tmp[5] = {0x0};
//      // sprintf(tmp,"%d %2d",i,loop);
//      // write(1,tmp, sizeof(tmp));
//      // write(1,"\n",1);
//      green_mutex_lock(&mutex);
//      int local = global +1;
//      green_mutex_unlock(&mutex);
//      //To show not perfect thing
//      //printf("AJAJ %d: %d\n", i, loop);
//
//     for(int j = 0; j<100000;j++){j = j+2; j--;}
//     green_mutex_lock(&mutex);
//     global = local;
//     green_mutex_unlock(&mutex);
//     loop--;
//     //green_yeild();
//   }
// }

  void *test(void *arg){
    int id = *(int*) arg;
    int loop = 1000;
    while(loop > 0){
      green_mutex_lock(&mutex);
      while(flag != id){
        green_cond_wait(&cond, &mutex);
      }
      global++;
      flag = (id + 1) % 2;
      green_cond_signal(&cond);
      green_mutex_unlock(&mutex);
      loop--;
    }
  }


// void* testSharedResource(void* arg) {
//   int id = *(int*)arg;
//   for(int i = 0; i < 10000; i++) {
//     green_mutex_lock(&mutex);
//     int temp = counter;
//
//     int dummy = 0;
//     if(temp > 1337) // waste time between read and write
//       dummy++;
//
//     temp++;
//     counter = temp;
//     green_mutex_unlock(&mutex);
//   }
// }
//
// int buffer;
// int productions;
// //green_cond_t full, empty;
// void produce() {
//   for(int i = 0; i < productions/(numThreads/2); i++) {
//     green_mutex_lock(&mutex);
//     while(buffer == 1) // wait for consumer before producing more
//       green_cond_wait(&cond);
//     buffer = 1;
//     //printf("Produced!\n");
//     green_cond_signal(&full);
//     green_mutex_unlock(&mutex);
//   }
// }

//
// void *test(void *arg){
//   int id = *(int*) arg;
//   int loop = 4;
//   while(loop > 0) {
//     if(flag == id){
//       printf("thread %d: %d\n", id, loop);
//
//       //green_cond_print(&cond);
//       loop--;
//       flag = (id + 1) % 2;
//       green_cond_signal(&cond);
//     }else{
//       green_cond_wait(&cond);
//     }
//   }
// }

int main(){
  green_t g0, g1;
  int a0 = 0;
  int a1 = 1;
  green_cond_init(&cond);
  green_cond_init(&full);
  green_mutex_init(&mutex);
  green_create(&g0, test, &a0);
  //green_queue_print();
  green_create(&g1, test, &a1);
  //green_queue_print();
  // green_create(&g0, testSharedResource, &a0);
  // green_create(&g1, testSharedResource, &a1);

  //produce();
  green_join(&g0, NULL);
  green_join(&g1,NULL);
  printf("%d\n",global);
  printf("done\n");
  return 0;
}
