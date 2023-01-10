#include "utils_3.1.h"

#define N 15 // array size
#define M 4  // number of tulkuns
#define T 50 // seconds of movement

typedef struct argTulkun
{
    pthread_t tid;
    uint seed;
    int direction;
    int position;
    int *pCircle;
    int circle_size;
    pthread_mutex_t *mxCircle;
} argTulkun_t;

volatile sig_atomic_t last_signal = 0;

void sigalrm_handler(int sig);
void read_arguments(int argc, char **argv, int *circle_size, int *tulkuns_no, int *secs);
void *tulkun_walk(void *args);
void print_array(int *array, int array_size);
void create_tulkuns(argTulkun_t *args, int *pCircle, pthread_mutex_t *mxCircle,
                    int tulkuns_no, int circle_size);
void execute(int *pCircle, pthread_mutex_t *mxCircle, int secs, int circle_size);

int main(int argc, char **argv)
{
    int circle_size, tulkuns_no, secs;
    read_arguments(argc, argv, &circle_size, &tulkuns_no, &secs);

    // Initializing array representing circle and array of mutexes.
    argTulkun_t *args_array = malloc(tulkuns_no * sizeof *args_array);

    int *pCircle = malloc(circle_size * sizeof *pCircle);
    if (NULL == pCircle)
        ERR("malloc");
    memset(pCircle, 0, circle_size * sizeof(int));

    pthread_mutex_t *mxCircle = malloc(circle_size * sizeof *mxCircle);
    for (int i = 0; i < circle_size; i++)
    {
        if (pthread_mutex_init(&mxCircle[i], NULL))
            ERR("mutex_init");
    }

    create_tulkuns(args_array, pCircle, mxCircle, tulkuns_no, circle_size);
    execute(pCircle, mxCircle, secs, circle_size);

    for (int i = 0; i < tulkuns_no; i++)
        pthread_cancel(args_array[i].tid);

    for (int i = 0; i < tulkuns_no; i++)
        if (pthread_join(args_array[i].tid, NULL))
            ERR("pthread_join");

    print_array(pCircle, circle_size);

    free(mxCircle);
    free(pCircle);
    free(args_array);
    return EXIT_SUCCESS;
}

void sigalrm_handler(int sig)
{
    last_signal = sig;
}

void read_arguments(int argc, char **argv, int *circle_size, int *tulkuns_no, int *secs)
{
    *circle_size = N;
    *tulkuns_no = M;
    *secs = T;

    if (argc > 1)
    {
        *circle_size = atoi(argv[1]);
        if (*circle_size < 5 || *circle_size > 500)
        {
            fprintf(stderr, "Array size should be in [5, 500]\n");
            exit(EXIT_FAILURE);
        }
        if (argc > 2)
        {
            *tulkuns_no = atoi(argv[2]);
            if (*tulkuns_no < 3 || *tulkuns_no > 300)
            {
                fprintf(stderr, "Array size should be in [3, 300]\n");
                exit(EXIT_FAILURE);
            }
            if (argc > 3)
            {
                *secs = atoi(argv[3]);
                if (*secs < 1)
                {
                    fprintf(stderr, "Number of seconds should be >1\n");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
}

void create_tulkuns(argTulkun_t *args_array, int *pCircle, pthread_mutex_t *mxCircle, int tulkuns_no, int circle_size)
{
    srand(time(NULL));
    for (int i = 0; i < tulkuns_no; i++)
    {
        args_array[i].pCircle = pCircle;
        args_array[i].circle_size = circle_size;
        args_array[i].seed = rand();
        args_array[i].direction = randint(0, 1) * 2 - 1;
        args_array[i].mxCircle = mxCircle;

        int pos;
        do
        {
            pos = randint(0, circle_size - 1);
        } while (1 == args_array->pCircle[pos] && tulkuns_no <= circle_size);

        args_array[i].position = pos;
        args_array->pCircle[pos] = 1;

        if (pthread_create(&args_array[i].tid, NULL, tulkun_walk, &args_array[i]))
            ERR("pthread_create");
    }
}

void *tulkun_walk(void *_args)
{
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    argTulkun_t *args = _args;
    // uint sleepytime = randint_t(&args->seed, 10, 1000);
    // millisleep(sleepytime);

    int new_position;
    // fprintf(stderr, "pos_new: %d\n", args->position);
    for (;;)
    {
        new_position = (args->circle_size + args->position + args->direction) % args->circle_size;
        pthread_mutex_lock(&args->mxCircle[new_position]);
        if (1 == args->pCircle[new_position])
        {
            pthread_mutex_unlock(&args->mxCircle[new_position]);
            args->direction = -args->direction;
        }
        else
        {
            pthread_mutex_unlock(&args->mxCircle[new_position]);

            pthread_mutex_lock(&args->mxCircle[args->position]);
            args->pCircle[args->position] = 0;
            pthread_mutex_unlock(&args->mxCircle[args->position]);

            args->position = new_position;

            pthread_mutex_lock(&args->mxCircle[args->position]);
            args->pCircle[args->position] = 1;
            pthread_mutex_unlock(&args->mxCircle[args->position]);
        }
        sleep(1);
    }

    return NULL;
}

void execute(int *pCircle, pthread_mutex_t *mxCircle, int secs, int circle_size)
{
    sethandler(sigalrm_handler, SIGALRM);
    sethandler(sigalrm_handler, SIGINT);
    alarm(secs);
    while (last_signal != SIGALRM)
    {
        if (last_signal == SIGINT)
        {
            break;
        }
        else if (last_signal && last_signal != SIGALRM)
        {
            fprintf(stderr, "Unexpected signal: %d\n", last_signal);
            exit(EXIT_FAILURE);
        }

        pthread_mutex_lock(mxCircle);
        print_array(pCircle, circle_size);
        pthread_mutex_unlock(mxCircle);
        struct timespec t = {1, 0};
        safesleep(&t);
    }
}

void print_array(int *array, int array_size)
{
    printf("[ ");
    for (int i = 0; i < array_size; i++)
    {
        if (0 == array[i])
            printf("_ ");
        else
            printf("%d ", array[i]);
        // if (2 == i % 3) printf(". ");
    }
    printf("]\n");
}