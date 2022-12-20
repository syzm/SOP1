#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#define MAX_N 128

#define ERR(source) \
	(fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

volatile sig_atomic_t last_signal = 0, loc_counter = 0;
int pids[MAX_N]; int n;

void usage(char *name){
    fprintf(stderr, "USAGE: %s N\n", name);
    fprintf(stderr, "N - number of children\n");
    exit(EXIT_FAILURE);
}

void sethandler(void (*f)(int), int sigNo){
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f;
	if (-1 == sigaction(sigNo, &act, NULL))
		ERR("sigaction");
}

void sig_handler(int sig){
	last_signal = sig;
}

void parent_sigint_handler(int sig){
    last_signal = sig;
    for(int i = 0; i < n; i++)
        kill(pids[i], SIGINT);
}

void child_sigint_handler(int sig){

    last_signal = sig;

    int out;
	ssize_t count = 0; 
    char name[MAX_N];
    char buf[MAX_N];

	sprintf (buf, "%d", (int)loc_counter);
		
    sprintf(name, "place/%d.txt", getpid());
    
	if ((out = open(name, O_WRONLY | O_TRUNC | O_CREAT | O_APPEND, 0777)) < 0)
		ERR("open");

	if ((count = write(out, buf, strlen(buf))) < 0)
		ERR("write");

	if (close(out))
		ERR("close");
    
    exit(EXIT_SUCCESS);

}

void child_work(sigset_t oldmask){

    sethandler(child_sigint_handler, SIGINT);
    printf("[%d]\n", getpid());
    srand(getpid());

    while(1){
        last_signal = 0;
        while(last_signal != SIGUSR1) 
            sigsuspend(&oldmask);
        while(last_signal != SIGUSR2){
            int s = rand() % 101 + 100;
            struct timespec t = { 0, s * 1000000};
            nanosleep(&t, NULL);
            printf("[%d] %d\n", getpid(), ++loc_counter);
        }
    }
}

void parent_work(sigset_t oldmask){

    sethandler(parent_sigint_handler, SIGINT);
    int i = 0;
    kill(pids[0], SIGUSR1);
    
    while(last_signal != SIGINT){
        last_signal = 0;
        while(last_signal != SIGUSR1 && last_signal != SIGINT)
            sigsuspend(&oldmask);
        kill(pids[(i)%n], SIGUSR2);
        if(last_signal == SIGINT) break;
        kill(pids[(++i)%n], SIGUSR1);
    }
    return;
}


int main(int argc, char **argv){

    if(argc < 2){
        usage(argv[0]);
	}
    n = atoi(argv[1]);
    printf("%d\n", getpid());

    sethandler(sig_handler, SIGUSR1);
    sethandler(sig_handler, SIGUSR2);

    sigset_t mask, oldmask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGINT);

	sigprocmask(SIG_BLOCK, &mask, &oldmask);

    for(int i = 0; i < n; i++){

        pid_t pid;
        if ((pid = fork()) < 0)
            ERR("fork");
        if (0 == pid){
            child_work(oldmask);
		}
        else{
            pids[i] = pid;
        }
    }
    parent_work(oldmask);
	while (wait(NULL) > 0)
		;
	return EXIT_SUCCESS;
}