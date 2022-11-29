#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))
#define MAX_PATH 100



void usage(char *pname) {
    fprintf(stderr, "Usage: %s [OPTION]...\n"
                    "Listuje pliki i ich rozmiary w podanych katalogach.\n"
                    "  -p KATALOG\t\tkatalog do wylistowania\n"
                    "  -e ROZSZERZENIE\t\t(opcjonalne) pomin pliki z danym rozszerzeniem\n"
                    "  -d LICZBA\t\t\tglebokosc skanowania\n"
                    "  -o PLIK\t\tzapisuje wyjscie programu w PLIK\n"
            , pname);
    exit(EXIT_FAILURE);
}


int has_extension(const char *filename, const char *ext)
{
    if('.' != filename[strlen(filename) - strlen(ext) - 1])
    {
        return 0;
    }
    return !strcmp(filename + strlen(filename) - strlen(ext), ext);
}

void list_dir(char* dirName, FILE* out, int depth, char* ext){
    DIR *dirp;
    struct dirent *dp;
    struct stat filestat;


    if (NULL == (dirp = opendir(dirName))) {
        if (errno == ENOENT) {
            fprintf(stderr, "No such file or directory: %s\n", dirName);
            return;
        }
        if (errno == EACCES)
        {
            fprintf(stderr, "No access: %s\n", dirName);
            return;
        }
        ERR("opendir");
    }

    fprintf(out, "path:\t%s\n", dirName);
    do {
        errno = 0;
        if ((dp = readdir(dirp)) != NULL) {
            char path[MAX_PATH];

            if (dp->d_name[0] == '.')
                continue;

            if (snprintf(path, MAX_PATH, "%s/%s", dirName, dp->d_name) >= MAX_PATH) {
                fprintf(stderr, "Path too long! %s\n", path);
                if (closedir(dirp))
                    ERR("closedir");
                return;
            }

            if (lstat(path, &filestat)) {
                fprintf(stderr, "%s\n", path);
                ERR("lstat");
            }

            if(depth > 1 && S_ISDIR(filestat.st_mode))
            {
                list_dir(path, out, depth - 1, ext);
            }

            if(ext != NULL && has_extension(dp->d_name, ext))
                continue;

            if(!S_ISDIR(filestat.st_mode))
                fprintf(out, "%s %ld\n", dp->d_name, filestat.st_size);
        }
    } while (dp != NULL);

    if (errno != 0)
        ERR("readdir");
    if (closedir(dirp))
        ERR("closedir");
}


int main(int argc, char **argv)
{
    char c;
    FILE *out = stdout;
    char *env = getenv("L1_OUTPUTFILE");
    int depth = 1;
    char* ext = NULL;

    while((c = getopt(argc, argv, "p:oe:d:")) != -1)
    {
        switch(c)
        {
            case 'o':
                if(out != stdout)
                {
                    usage(argv[0]);
                }
                if ((out = fopen(env, "w")) == NULL)
                    ERR("fopen");
                break;
            case 'd':
                depth = atoi(optarg);
                break;
            case 'e':
                ext = optarg;
                break;
            case 'p':
                break;
            case '?':
            default:
                usage(argv[0]);

        }
    }

    if (argc > optind) {
        usage(argv[0]);
    }

    optind = 0;

    while((c = getopt(argc, argv, "p:oe:d:")) != -1)
    {
        switch(c)
        {
            case 'p':
                list_dir(optarg, out, depth, ext);
            case 'o':
                break;
            case 'd':
                break;
            case 'e':
                break;
            case '?':
            default:
                usage(argv[0]);

        }
    }


    if (argc > optind) {
        usage(argv[0]);
    }

    if (out != stdout && fclose(out))
        ERR("fclose");

    return EXIT_SUCCESS;

}