#ifndef SEMAPHORE
#define SEMAPHORE

typedef struct
{
    int value;
    int max_value;

    struct task *queue;
} semaphore;

semaphore *new_semaphore(int max_value);

void wait_semaphore(semaphore *s);
void release_semaphore(semaphore *s);

#endif
