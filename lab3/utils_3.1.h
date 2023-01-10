#define _GNU_SOURCE // TEMP_FAILURE_RETRY
#ifndef _UTLIS_H
#define _UTLIS_H

// 9.01.2023 21:44

// COLOR THEME: Linux Themes for VS Code => United Ubuntu
//              Dracula Official => Dracula

#include <stdio.h>    // input/output
#include <stdlib.h>   // EXIT_SUCCESS/FAILURE, get-, put-, setenv
#include <string.h>   // string
#include <unistd.h>   // getopt, getcwd, chdir, sleep
#include <dirent.h>   // dirent
#include <sys/stat.h> // stat, mkdir
#include <errno.h>    // errno
#include <signal.h>   // signal hadling
#include <time.h>     // time
#include <sys/wait.h> // wait, waitpid
#include <fcntl.h>    // file control
#include <pthread.h>  // threads
#include <math.h>     // duh

#define STR(X) _STR(X)
#define _STR(X) #X

#define ERR(source) (perror(source), /*kill(0, SIGKILL),*/           \
                     fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), \
                     exit(EXIT_FAILURE))

#define PUTENV(string)       \
    if (putenv(string) != 0) \
    perror("putenv")

#define MAX_PATH 256

typedef unsigned int uint;

// ===================================== L3 -- threads =====================================

// Multithread-safe generation of a random double between [0, 1]
double randdouble_t(uint *seed)
{
    return (double)rand_r(seed) / (double)RAND_MAX;
}

// Multithread-safe generation of a random integer in the range [min, max]
int randint_t(uint *seed, int min, int max)
{
    return min + (int)rand_r(seed) % (max - min + 1);
}

// Uses nanosleep and makes sure it slept the proper amount of time
int safesleep(struct timespec *ts)
{
    int res;
    do
    {
        res = nanosleep(ts, ts);
    } while (res && errno == EINTR);

    return res;
}

// Sleep for 'millisec' ms. Makes sure to sleep the requested amount. Safe with SIGALRM
int millisleep(uint millisec)
{
    time_t sec = millisec / 1e3;
    millisec = millisec - sec * 1e3;

    struct timespec req_time =
        {.tv_sec = sec,
         .tv_nsec = millisec * 1e6L};

    return safesleep(&req_time);
}

// Example of a function
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

// ===================================== L2 -- signals =====================================
// Global variable whose modification can't be interrupted by an arrival of a signal.
volatile sig_atomic_t some_var = 0;

// Sets the current process's (and the child processes') handling of the signal sigNo to the action specified in f.
void sethandler(void (*f)(int), int sigNo)
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = f;
    if (-1 == sigaction(sigNo, &act, NULL))
        ERR("sigaction");
}

// Example of handling a signal
void sig_handler(int sig)
{
    some_var++;
}

// Example action of handling a SIGCHLD: wait for the death of any process with the same process group.
// Flag WNOHANG makes the function return if the child isn't yet dead.
void sigchld_handler(int sig)
{
    pid_t pid;
    for (;;)
    {
        sleep(3);
        pid = waitpid(0, NULL, WNOHANG);
        if (pid == 0)
            return;
        if (pid <= 0)
        {
            if (errno == ECHILD)
                return;
            ERR("waitpid");
        }
    }
}

// Code snippet of manipulating the signal mask.
void signal_mask_example()
{
    sigset_t newmask, oldmask;
    // Initialize the mask to be empty
    sigemptyset(&newmask);
    // Add SIGUSR1 to mask.
    sigaddset(&newmask, SIGUSR1);
    // Change the signal mask of the calling thread:
    // Resulting set is the union of the current set and 'newmask'.
    // SIGUSR1 will be added to current signal mask. Previous mask is stored in 'oldmask'.
    // (!) For multi-threaded programs: use pthread_sigmask
    sigprocmask(SIG_BLOCK, &newmask, &oldmask);

    /* ... */

    volatile sig_atomic_t last_signal = 0; /* To do! This should be a global var */
    // Wait for SIGUSR2
    while (last_signal != SIGUSR2)
        sigsuspend(&oldmask);
    // Alternative: sigwait. There are some differences
    // The program is effectively suspended
    // until one of the signals that is not a member of 'oldmask' arrives

    /* ... */

    // Resulting set is the intersection of the current set and complement 'newmask'.
    sigprocmask(SIG_UNBLOCK, &newmask, NULL);
}

// Example procedure of creating child processes
void create_children(int n)
{
    pid_t s;
    while (n-- > 0)
    {
        switch (s = fork())
        {
        case 0:
            // sethandler(sig_handler, SIGUSR1);
            // child_labour();
            exit(EXIT_SUCCESS);
        case -1:
            perror("Fork:");
            exit(EXIT_FAILURE);
        default: // s > 0
            printf("Child [%d] created\n", s);
            // s -- pid of the child
        }
    }
}

// Print the number of remaining child processes in 3 seconds intervals.
void check_remaing_children(int n)
{
    while (n > 0)
    {
        sleep(3);
        pid_t pid;
        for (;;)
        {
            pid = waitpid(0, NULL, WNOHANG);
            if (pid > 0)
                n--;
            if (pid == 0) // temporarily no ended children
                break;
            if (pid <= 0)
            {
                if (ECHILD == errno) // permanently no children
                    break;
                ERR("waitpid");
            }
        }
        printf("Parent: %d processes remain\n", n);
    }
}

// Reads 'count' bytes from the file associated with the file descriptor 'fd'
// into the buffer pointed to by 'buf', making sure the action isn't interrupted by an arrival of a signal.
ssize_t bulk_read(int fd, char *buf, size_t count)
{
    ssize_t c;
    ssize_t len = 0;

    do
    {
        c = TEMP_FAILURE_RETRY(read(fd, buf, count));
        if (c < 0)
            return c;
        if (c == 0) // EOF
            return len;
        buf += c; // move the pointer
        len += c;
        count -= c;
    } while (count > 0);

    return len;
}

// Writes 'count' bytes to the file associated with the file descriptor 'fd'
// from the buffer pointed to by 'buf', making sure the action isn't interrupted by an arrival of a signal.
ssize_t bulk_write(int fd, char *buf, size_t count)
{
    ssize_t c;
    ssize_t len = 0;
    do
    {
        c = TEMP_FAILURE_RETRY(write(fd, buf, count));
        if (c < 0)
            return c;
        buf += c; // move the pointer
        len += c;
        count -= c;
    } while (count > 0);
    return len;
}

// Transfers 'b' blocks of 's' bytes from /dev/urandom to file specified by 'name'.
void transfer_blocks(int b, int s, char *name)
{
    int in, out;
    ssize_t count;
    char *buf = (char *)malloc(s);
    if (!buf)
        ERR("malloc");
    if ((out = TEMP_FAILURE_RETRY(open(name, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0777))) < 0)
        ERR("open");
    if ((in = TEMP_FAILURE_RETRY(open("/dev/urandom", O_RDONLY))) < 0)
        ERR("open");
    for (int i = 0; i < b; i++)
    {
        if ((count = bulk_read(in, buf, s)) < 0)
            ERR("read");
        if ((count = bulk_write(out, buf, count)) < 0)
            ERR("read");
        if (TEMP_FAILURE_RETRY(
                fprintf(stderr, "Block of %ld bytes transfered.\n", count)) < 0)
            ERR("fprintf");
    }
    if (TEMP_FAILURE_RETRY(close(in)))
        ERR("close");
    if (TEMP_FAILURE_RETRY(close(out)))
        ERR("close");
    free(buf);
    if (kill(0, SIGUSR1))
        ERR("kill");
}

// ====================== L1 -- directories, files, POSIX environment ======================

extern char *optarg;
extern int opterr, optind, optopt;

// Scans current directory, displays number of dirs, files, links and other
void scan_cwd()
{
    DIR *dirp;            // represents a dir stream == ordered sequence
                          // of all dir entries in a dir
    struct dirent *dp;    // dirent {inode (d_ino), name (d_name)}
    struct stat filestat; // file info

    int dirs = 0, files = 0,
        links = 0, other = 0;

    if (NULL == (dirp = opendir(".")))
        ERR("opendir");

    do
    {
        errno = 0; // manually set because readdir doesn't,
                   // readdir returns NULL as end of dir AND as error
        // dp (directory entry)
        // dirp (current position in dirstream, set by readdir to next pos)
        if ((dp = readdir(dirp)) != NULL) // iteration happens here
        {

            if (lstat(dp->d_name, &filestat))
                ERR("lstat");
            if (S_ISDIR(filestat.st_mode))
                dirs++;
            else if (S_ISREG(filestat.st_mode))
                files++;
            else if (S_ISLNK(filestat.st_mode))
                links++;
            else
                other++;
        }
    } while (dp != NULL);

    if (errno != 0)
        ERR("readdir");
    if (closedir(dirp)) // REMEMBER!
        ERR("closedir");
    printf("Files: %d, Dirs: %d, Links: %d, Other: %d\n",
           files, dirs, links, other);
}

// Lists files, dirs etc. in the current working directory
void cwd_listing(char *dirname, FILE *out)
{
    DIR *dirp;
    struct dirent *dp;
    struct stat filestat;
    char path[MAX_PATH];

    if (getcwd(path, MAX_PATH) == NULL)
        ERR("getcwd");

    if (NULL == (dirp = opendir(dirname)))
        ERR("opendir");

    fprintf(out, "PATH:\n%s\nLIST:\n", dirname);

    if (chdir(dirname))
    {
        if (errno == ENOENT)
        {
            fprintf(stderr, "No such file or directory: %s", dirname);
            return;
        }
        ERR("chdir");
    }

    do
    {
        errno = 0;
        if ((dp = readdir(dirp)) != NULL)
        {
            if (dp->d_name[0] == '.')
                continue;

            if (lstat(dp->d_name, &filestat))
                ERR("lstat");

            fprintf(out, "\t%s\t\t%ld\n", dp->d_name, filestat.st_size);
        }
    } while (dp != NULL);

    if (errno != 0)
        ERR("readdir");
    if (closedir(dirp))
        ERR("closedir");
    if (chdir(path))
        ERR("chdir");
}

// Scan dir_to_scan using scan_current_dir()
void scan_dir(const char *work_dir, const char *dir_to_scan)
{
    if (chdir(dir_to_scan))
        ERR("chdir");

    printf("%s:\n", dir_to_scan);
    scan_cwd();
    if (chdir(work_dir))
        ERR("chdir");
}

// main() code snippet for setting the program flags
/*
    while ((c = getopt(argc, argv, "p:o:")) != -1)
    {
        switch (c)
        {
        case 'p':
            break;

        case 'o':
            if (out != stdout)
                usage(argv[0]);
            if ((out = fopen(optarg, "w")) == 0)
                ERR("fopen");
            break;

        case '?':
        default:
            usage(argv[0]);
            break;
        }
    }

    while ((c = getopt(argc, argv, "p:o:")) != -1)
    {
        switch (c)
        {
        case 'p':
            cwd_listing(optarg, out);
            break;

        case 'o':
            break;

        case '?':
        default:
            usage(argv[0]);
            break;
        }
    }
*/

// ===================================== Miscellaneous =====================================
// Prints correct usage of the called program
void usage(char *prog_name)
{
    fprintf(stderr, "Usage: %s ... \n", prog_name);
    exit(EXIT_FAILURE);
}

// Returns random integer in the range [min, max]
// Necessary srand()!
int randint(int min, int max)
{
    return min + rand() % (max - min + 1);
}

// Returns random double between [0, 1]
// Necessary srand()!
double randdouble()
{
    return (double)rand() / (double)RAND_MAX;
}

// Returns random double between [min, max]
// Necessary srand()!
double randrange(double min, double max)
{
    return min + (double)rand() / ((double)RAND_MAX / (max - min));
}
#endif