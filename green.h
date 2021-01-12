#include <ucontext.h>

typedef struct green_t{
  ucontext_t *context;
  void *(*fun)(void*);
  void *arg;
  struct green_t *next;
  struct green_t *join;
  void *retval;
  int zombie;
} green_t;

typedef struct green_cond_t{
   green_t *head;
   green_t *tail;
}green_cond_t;

typedef struct green_mutex_t{
  volatile int taken;
  green_t *queue;
}green_mutex_t;

int green_create(green_t *thread, void *(*fun)(void*), void *arg);
int green_yeild();
int green_join(green_t *thread, void** val);
void green_thread();
void green_cond_init(green_cond_t*);
int green_cond_wait(green_cond_t*, green_mutex_t*);
void green_cond_signal(green_cond_t*);
void green_queue_print();
void green_cond_print(green_cond_t*);

int green_mutex_init(green_mutex_t *mutex);
int green_mutex_lock(green_mutex_t *mutex);
int green_mutex_unlock(green_mutex_t *mutex);
