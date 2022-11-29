#define _XOPEN_SOURCE 500
#include <dirent.h>
#include <errno.h>
#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#define MAX_NAME 255
#define MAX_PATH 255


#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

void usage(char *pname){
	fprintf(stderr, "USAGE: \n%1$s add <AUTHOR> <BOOK>\n%1$s list <AUTHOR> \n%1$s stats \n%1$s save <FILE>\n", pname);
	exit(EXIT_FAILURE);
}

void stats(FILE* stream){

        DIR* dirp, *author;
        struct dirent *dp, *dp2;
        struct stat filestat;
        char cwd[MAX_PATH];
        int books = 0;

        if(getcwd(cwd, sizeof(cwd)) == NULL)
		    ERR("getcwd");
        if (NULL == (dirp = opendir("."))){
		    ERR("opendir");
        }
        while ((dp = readdir(dirp)) != NULL) {
            
            if(dp->d_name[0] == '.') 
                continue;
            
            if( stat(dp->d_name, &filestat) ){
                ERR("stat");
            }

            errno = 0;
            if (S_ISDIR(filestat.st_mode)){
                books = 0;
                fprintf(stream,"%s: ", dp->d_name);
                if(chdir(dp->d_name) == -1) {
                    fprintf(stderr, "Could not change directory to %s\n", dp->d_name);
                    ERR("chdir");
                }

                if (NULL == (author = opendir("."))){
		            ERR("opendir");
                }
                
                while ((dp2 = readdir(author)) != NULL) {
                    if(dp2->d_name[0] == '.') 
                        continue;
                    books++;
                }
                closedir(author);

                if(chdir(cwd) == -1) 
		            ERR("chdir");

                fprintf(stream,"%d\n", books);
            }
        }
        closedir(dirp);
}

int list(char* author){
    DIR* dirp;
    struct dirent *dp;
    struct stat filestat;

    if(chdir(author) == -1) {
        fprintf(stderr, "No such author: %s\n", author);
        return 1;
    }
    if (NULL == (dirp = opendir("."))){
        ERR("opendir");
        
    }
    if (dirp) {
        while ((dp = readdir(dirp)) != NULL) {
            if(dp->d_name[0] == '.') 
                continue;
            if( stat(dp->d_name, &filestat) )
                ERR("stat");
            printf("%10s, %20ld\n", dp->d_name, filestat.st_mtime);
        }
        closedir(dirp);
    }
    return 0;
}

int main(int argc, char **argv){

    char* author, *book; char cwd[MAX_PATH]; FILE* f;

    if(getcwd(cwd, sizeof(cwd)) == NULL)
        ERR("getcwd");

    if(!strcmp(argv[1], "add")) { 

        if(argc != 4)
            usage(argv[0]);

        author = argv[2];
        book = argv[3];

        struct stat filestat;

        if (stat(author, &filestat) == -1) {
            if(mkdir(author, 0777) == -1)
                ERR("mkdir");
        }
        
        if(chdir(author) == -1) {
            ERR("chdir");
        }
        if(fopen(book, "w+") == NULL){
            ERR("fopen");
        }
        
    }
    else if(!strcmp(argv[1], "list")) {

        if(argc != 3)
            usage(argv[0]);
        
        author = argv[2];

        if(list(author))
            return 1;
        
        
    }
    else if(!strcmp(argv[1], "stats")) {
        
        if(argc != 2)
            usage(argv[0]);

        stats(stdout);


    }
    else if(!strcmp(argv[1], "save")) {
        
        if(argc != 3)
            usage(argv[0]);
        char* fname = argv[2];

        if((f = fopen(fname, "w+")) == NULL)
            fprintf(stderr, "NO SUCH FILE OR DIRECTORY: %s\n", fname);
        stats(f);
    }
    else{
        fprintf(stderr, "INVALID ARGUMENT %s\n", argv[1]);
        usage(argv[0]);
        return 1;
    }
    return 0;

}