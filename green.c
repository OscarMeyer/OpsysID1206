#include <stdlib.h>
#include <ucontext.h>
#include <assert.h>
#include <signal.h>
#include <sys/time.h>
#include "green.h"

#define FALSE 0
#define TRUE 1

#define STACK_SIZE 4096
#define PERIOD 100

static sigset_t block;


static ucontext_t main_cntx = {0};
//Will probebly have to change a or several NULL
static green_t main_green = {&main_cntx, NULL, NULL, NULL, NULL, NULL,FALSE};
static green_t *running = &main_green;
static green_t *head = NULL;
static green_t *tail = NULL;

static void init() __attribute__((constructor));
void timer_handler(int);

void insert(green_t* new){
  new -> next = NULL;
  if(head == NULL) {
    head = new;
  } else
    tail -> next = new;

  tail = new;
}

green_t* pop(){
  green_t *toPop = head;
  if(head != NULL)
    head = head -> next;
  // toPop -> next = NULL;

  return toPop;
}

void init(){
  sigemptyset(&block);
  sigaddset(&block, SIGVTALRM);

  struct sigaction act = {0};
  struct timeval interval;
  struct itimerval period;

  act.sa_handler = timer_handler;
  assert(sigaction(SIGVTALRM, &act, NULL) == 0);
  interval.tv_sec = 0;
  interval.tv_usec = PERIOD;
  period.it_interval = interval;
  period.it_value = interval;
  setitimer(ITIMER_VIRTUAL, &period, NULL);
  getcontext(&main_cntx);

}

void timer_handler(int sig){
  //printf("%p\n", head);
  green_yeild();
}
int green_mutex_init(green_mutex_t *mutex){
  mutex -> taken = FALSE;
  mutex -> queue = NULL;
}

int green_mutex_lock(green_mutex_t *mutex){
  sigprocmask(SIG_BLOCK, &block, NULL);

  green_t* susp = running;

  while(mutex -> taken){
    next_running();
    swapcontext(susp->context, running->context);
    if(mutex -> queue != NULL)
      susp -> next = mutex -> queue;
    else
      susp -> next = NULL;
    mutex -> queue = susp;
  }
  mutex -> taken = TRUE;
  sigprocmask(SIG_BLOCK, &block, NULL);
  return 0;
}
int green_mutex_unlock(green_mutex_t* mutex){
  sigprocmask(SIG_BLOCK, &block, NULL);

  if(mutex -> queue != NULL){
    green_t* current = mutex -> queue;
    while(current -> next != NULL){
      current = current -> next;
    }
    current -> next = head;
    if(tail == NULL)
      tail = current;
    head = mutex -> queue;
    mutex -> queue = NULL;
  }

  mutex -> taken = FALSE;
  sigprocmask(SIG_BLOCK, &block, NULL);
  return 0;
}

int green_create(green_t *new, void *(*fun)(void*), void *arg){
  ucontext_t *cntx = (ucontext_t *)malloc(sizeof(ucontext_t));
  getcontext(cntx);

  void *stack = malloc(STACK_SIZE);

  cntx -> uc_stack.ss_sp = stack;
  cntx -> uc_stack.ss_size = STACK_SIZE;
  makecontext(cntx, green_thread, 0);

  new -> context = cntx;
  new -> fun = fun;
  new -> arg = arg;
  new -> next = NULL;
  new -> join = NULL;
  new -> retval = NULL;
  new -> zombie = FALSE;
  sigprocmask(SIG_BLOCK, &block, NULL);
  insert(new);
  sigprocmask(SIG_UNBLOCK, &block, NULL);
  return 0;
}

void green_thread() {
  // printf("green_thread() start\n");
  green_t *this = running;
  //green_queue_print();
  void *result = (*this->fun)(this->arg);
  sigprocmask(SIG_BLOCK, &block, NULL);
  //This may be broken...
  if(this -> join != NULL)
    insert(this->join);

  this -> retval = result;
  this -> zombie = TRUE;
  // green_queue_print();
  // printf("Innan next running\n");
  next_running();
   //green_queue_print();
  // printf("Efter next running\n");
  // green_thread_print(running);
  setcontext(running -> context);
  // printf("efter context\n");
  sigprocmask(SIG_UNBLOCK, &block, NULL);
}

int green_yeild(){
  sigprocmask(SIG_BLOCK, &block, NULL);
  green_t * susp = running;

  insert(susp);

  next_running();
  swapcontext(susp->context, running->context);

  sigprocmask(SIG_UNBLOCK, &block, NULL);

  return 0;
}

int green_join(green_t *thread, void **res){
  if(!thread->zombie){
    green_t *susp = running;
    if(thread -> join == NULL){
      thread -> join = susp;
    } else {
      sigprocmask(SIG_BLOCK, &block, NULL);
      insert(susp);
      sigprocmask(SIG_UNBLOCK, &block, NULL);
    }
    running = thread;
    sigprocmask(SIG_BLOCK, &block, NULL);
    swapcontext(susp->context, running->context);
    sigprocmask(SIG_UNBLOCK, &block, NULL);
  }
  res = thread -> retval;
  free(thread->context);
  return 0;
}

void next_running(){
  running = pop();
  if(running != NULL) {
    while(running -> zombie == TRUE)
      running = pop();
  }
  // running -> next = NULL;
  if(running == NULL)
    write("AJAJ, No threads to be found.\n");
}

void green_cond_init(green_cond_t* cond){
  cond -> head = NULL;
  cond -> tail = NULL;
}
int green_cond_wait(green_cond_t* cond, green_mutex_t* mutex) {
  sigprocmask(SIG_BLOCK, &block, NULL); // block timer interupt

  // suspend thread on condition
  green_t* susp = running;
  if(cond -> head == NULL)
    cond -> head = susp;
  else
    cond -> tail -> next = susp;

  cond -> tail = susp;
  //printf("Innan mutex != NULL\n");
  if(mutex != NULL){
    // release the lock if we have a mutex
    // move suspended threads to ready queue

    if (mutex -> queue != NULL) {
      green_t* current = mutex -> queue;
      //printf("Innan while\n");
      while(current -> next != NULL){
        current = current -> next;
      }
      //printf("efter while\n");

      current -> next = head;
      head = mutex -> queue;
      if(tail == NULL)
        tail = current;
      mutex -> queue = NULL;
    }
    mutex -> taken = FALSE;
  }
  //printf("Efter mutex != NULL\n");


  // schedule the next thread
  next_running();
  swapcontext(susp->context, running->context);
  //printf("Efter next thread nr1\n");

  if(mutex != NULL) {
    // try to take the lock
    while(mutex -> taken) {
      //printf("Tog inte låset\n");
      // bad luck suspended
      next_running();
      swapcontext(susp->context, running->context);
      if(mutex -> queue != NULL)
        susp -> next = mutex -> queue;
      else
        susp -> next = NULL;
      mutex -> queue = susp;
    }
    // take the lock
    // printf("Tog låset\n");
    mutex -> taken = TRUE;
  }
  // printf("Slut\n");
  // unblock timer
  sigprocmask(SIG_UNBLOCK, &block, NULL);
  return 0;
}

void green_cond_signal(green_cond_t* cond){
  green_t* thread = cond -> head;
  if(cond -> head != NULL)
    cond -> head = cond -> head -> next;
  if(cond -> head == NULL)
    cond -> tail = NULL;
  if(thread != NULL){
    sigprocmask(SIG_BLOCK, &block, NULL);
    insert(thread);
    sigprocmask(SIG_UNBLOCK, &block, NULL);
  }
}
//Debugging stuff below
void green_queue_print() {
  sigprocmask(SIG_BLOCK, &block, NULL);
  printf("RNIG: %p\n\n", running);
  printf("HEAD: %p | next: %p\n", head, head -> next);
  if(head != NULL) {
    green_t* current = head -> next;
    if(current == NULL) printf("fail\n");
    while (current != NULL && current != tail) {
      printf("      %p | next: %p\n", current, current -> next);
      current = current -> next;
    }
  }
  printf("TAIL: %p | next: %p\n", tail, tail -> next);
  sigprocmask(SIG_BLOCK, &block, NULL);
}

void green_thread_print(green_t* thread) {
  printf("Address: %p\n", thread);
  printf("Context: %p\n", thread -> context);
  printf("Zombie:  %d\n", thread -> zombie);
  printf("Retval:  %p\n", thread -> retval);
}

void green_cond_print(green_cond_t* cond){
  green_t* current = cond -> head;
  printf("%p\n", cond -> head);
  while(current != NULL){
    printf("%p\n", current);
    current = current -> next;
  }
}
