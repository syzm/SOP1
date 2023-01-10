#include "utils_3.2.h"
#define THREAD_NO 5

void sigint_handler(int sig)
{
    fprintf(stderr, "Handler\n");
}
void *thread_work(void *args);

typedef struct arg
{
    pthread_t tid;
    int no;
    int block;
} arg;

int main(int argc, char **argv)
{
    arg *Args = malloc(THREAD_NO * sizeof *Args);

    sethandler(sigint_handler, SIGINT);

    for (int i = 0; i < THREAD_NO; i++)
    {
        Args[i].no = i + 1;
        if (0 == i)
            Args[i].block = 0;
        else
            Args[i].block = 1;

        pthread_create(&Args[i].tid, NULL, thread_work, &Args[i]);
    }

    sleep(5);

    for (int i = 0; i < THREAD_NO; i++)
    {
        pthread_join(Args[i].tid, NULL);
    }
    free(Args);
    return EXIT_SUCCESS;
}

void *thread_work(void *args)
{
    arg *Args = args;
    fprintf(stderr, "Thread %d working\n", Args->no);
    if (1 == Args->block)
    {
        sigset_t newmask;
        sigemptyset(&newmask);
        sigaddset(&newmask, SIGINT);
        pthread_sigmask(SIG_BLOCK, &newmask, NULL);

        fprintf(stderr, "Thread %d blocking SIGINT\n", Args->no);
    }
    else if (0 == Args->block)
        fprintf(stderr, "Thread %d not blocking SIGINT\n", Args->no);

    return NULL;
}