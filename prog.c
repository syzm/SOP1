#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <ftw.h>
#include <sys/stat.h>
#include <string.h>


#define MAX_PATH 101
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

void usage(char *pname) {
    fprintf(stderr, "Usage: %s [OPTION]...\n"
                    "Listuje pliki i ich rozmiary w podanych katalogach.\n"
                    "  -o PLIK\t\tzapisuje wyjscie programy w PLIK, max. 1\n"
                    "  -p KATALOG\t\tkatalog do wylistowania\n"
                    "  -r (opcjonalne)\t\tskanuje takze podkatalogi \n"
                    "  -s (opcjonalne)\t\tnie pomija symlinkow\n"
                    "  -n NAZWA\t\t pomija skanowanie plikow i katalogow rozpoczynajacych sie dana nazwa\n", pname);
    exit(EXIT_FAILURE);
}

int startswith(const char *pre, const char *str) {
    return strncmp(pre, str, strlen(pre)) == 0;
}

void list_dir(char *dirName, FILE *out, char *prefix, int recursive, int symlinks) {
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

            if (dp->d_name[0] == '.' || (prefix && startswith(prefix, dp->d_name)))
                continue;

            if (snprintf(path, MAX_PATH, "%s/%s", dirName, dp->d_name) >= MAX_PATH) {
                fprintf(stderr, "Path too long! %s\n", path);
                if (closedir(dirp))
                    ERR("closedir");
                return;
            }

            if (symlinks ? stat(path, &filestat) : lstat(path, &filestat)) {
                fprintf(stderr, "%s\n", path);
                ERR("lstat");
            }

            if(recursive && S_ISDIR(filestat.st_mode))
            {
                list_dir(path, out, prefix, recursive);
            }
            else
            {
                fprintf(out, "%s %ld\n", dp->d_name, filestat.st_size);
            }
        }
    } while (dp != NULL);

    if (errno != 0)
        ERR("readdir");
    if (closedir(dirp))
        ERR("closedir");
}

int main(int argc, char **argv) {
    char c;
    int recursive = 0;
    int symlinks = 0;
    FILE *out = stdout;
    char *prefix = NULL;

    while ((c = getopt(argc, argv, "p:o:rsn:")) != -1) {
        switch (c) {
            case 'o':
                if (out != stdout)
                    usage(argv[0]);
                if ((out = fopen(optarg, "w")) == NULL)
                    ERR("fopen");
                break;
            case 'p':
                break;
            case 'r':
                recursive = 1;
                break;
            case 'n':
                prefix = optarg;
                break;
            case 's':
                symlinks = 1;
                break;
            case '?':
            default:
                usage(argv[0]);
        }
    }

    if (argc > optind)
        usage(argv[0]);



    optind = 0;

    while ((c = getopt(argc, argv, "p:o:rsn:")) != -1) {
        switch (c) {
            case 'p':
                list_dir(optarg, out, prefix, recursive, symlinks);
                break;
            case 'o':
                break;
            case 'n':
                break;
            case 's':
                break;
            case 'r':
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