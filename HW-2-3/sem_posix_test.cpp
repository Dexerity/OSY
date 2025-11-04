//***************************************************************************
//
// Program example for subject Operating Systems
//
// Petr Olivka, Dept. of Computer Science, petr.olivka@vsb.cz, 2021
//
// Example of posix semaphores.
// The first process will creates two semaphores.
// One semaphore will protect artificial critical section.
// The second semaphore is used as process number counter.
// The process which exits last will clean semaphores.
//
//***************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <semaphore.h>
#include <vector>
#include <string>


#define BUFFER_SIZE 5

sem_t *g_sem_empty = nullptr;
sem_t *g_sem_full = nullptr;
sem_t *g_sem_mutex = nullptr;

std::vector<std::string> buffer;

//**************************************************************************

void producer(std::string item)
{
    sem_wait(g_sem_empty);
    sem_wait(g_sem_mutex);

    buffer.push_back(item);
    printf("Produced: %s\n", item.c_str());

    sem_post(g_sem_mutex);
    sem_post(g_sem_full);
}

void consumer(std::string* item)
{
    sem_wait(g_sem_full);
    sem_wait(g_sem_mutex);

    *item = buffer.back();
    buffer.pop_back();
    printf("Consumed: %s\n", item->c_str());

    sem_post(g_sem_mutex);
    sem_post(g_sem_empty);
}

void* producer_thread(void* arg)
{
    for (int i = 0; i < 10; ++i) 
    {
        producer("item_" + std::to_string(i));
        sleep(1);
    }

    return nullptr;
}

void* consumer_thread(void* arg)
{
    for (int i = 0; i < 10; ++i) 
    {
        std::string item;
        consumer(&item);
        sleep(2);
    }

    return nullptr;
}

//***************************************************************************

int main( int t_narg, char **t_args )
{
    g_sem_empty = sem_open("/sem_empty", O_CREAT, 0644, BUFFER_SIZE);
    g_sem_full = sem_open("/sem_full", O_CREAT, 0644, 0);
    g_sem_mutex = sem_open("/sem_mutex", O_CREAT, 0644, 1);

    pthread_t prod_thread, cons_thread;
    pthread_create(&prod_thread, nullptr, producer_thread, nullptr);
    pthread_create(&cons_thread, nullptr, consumer_thread, nullptr);

    pthread_join(prod_thread, nullptr);
    pthread_join(cons_thread, nullptr);

    sem_close(g_sem_empty);
    sem_close(g_sem_full);
    sem_close(g_sem_mutex);

    sem_unlink("/sem_empty");
    sem_unlink("/sem_full");
    sem_unlink("/sem_mutex");
    
    return 0;
}
