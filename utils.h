#ifndef KUZU_UTILS
#define KUZU_UTILS

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

extern char *optarg;
extern int opterr, optind, optopt;

/// Logging utils
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

#define PUTENV(string) if (putenv(string) != 0) perror("putenv")

/// File Utils
/*
 * Scan current working dir by counting all directories, files, links and other file types
 */
void scan_current_dir()
{
    DIR *dirp;
    struct dirent *dp;
    struct stat filestat;
    int dirs = 0, files = 0, links = 0, other = 0;

    if(NULL == (dirp = opendir(".")))
        ERR("opendir");

    do {
        errno = 0;
        if((dp = readdir(dirp)) != NULL) {
            if(lstat(dp->d_name, &filestat))
                ERR("lstat");

            if(S_ISDIR(filestat.st_mode))
                dirs++;
            else if(S_ISREG(filestat.st_mode))
                files++;
            else if(S_ISLNK(filestat.st_mode))
                links++;
            else
                other++;
        }
    } while (dp != NULL);

    if (errno != 0)
        ERR("readdir");
    if (closedir(dirp))
        ERR("closedir");

    printf("Files: %d, Dirs: %d, Links: %d, Other: %d\n", files, dirs, links, other);
}

/*
 * Scan dir_to_scan using scan_current_dir()
 */
void scan_dir(const char* work_dir, const char* dir_to_scan)
{
    if(chdir(dir_to_scan))
        ERR("chdir");

    printf("%s:\n", dir_to_scan);
    scan_current_dir();
    if(chdir(work_dir))
        ERR("chdir");
}

/// Random utils
/*
 * Returns random integer in the range [min, max]
 *
 * Uses rand(), so remember to use srand()!
 */
int randint(int min, int max)
{
    return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}

/*
 * Returns random double between [0, 1]
 */
double randdouble()
{
    return (double)rand() / (double) RAND_MAX;
}

/*
 * Returns random double between [min, max]
 */
double randrange(double min, double max)
{
    return min + (double)rand() / ((double) RAND_MAX / (max - min));
}
#endif



int has_extension(const char *filename, const char *ext)
{
    if('.' != filename[strlen(filename) - strlen(ext) - 1])
    {
        return 0;
    }
    return !strcmp(filename + strlen(filename) - strlen(ext), ext);
}

int string_equals(char*a, char*b)
{
if(strcmp(a, b) == 0) return 1;
else return 0;
}
