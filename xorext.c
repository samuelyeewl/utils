#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>
#include <errno.h>
#include <fnmatch.h>
#include <limits.h>

void usage(char* prog) {
    fprintf(stderr, "Usage:\n\
            %s [dir1] ext1 [dir2] ext2\n\
            [-g]: Glob for input files\n\
            [-p] n: Print n filenames. If n <= 0, prints all.\n", prog);
    return;
}

int main(int argc, char* argv[]) {
    // Argument parsing
    int c;
    char* globs = "*";
    int num_f = INT_MAX;
    bool print_names = false;

    char dir1[1024], dir2[1024];
    char ext1[256], ext2[256];

    while ((c = getopt(argc, argv, "g:p:")) != -1) {
        switch(c) {
            case 'g':
                globs = optarg;
                break;
            case 'p':
                print_names = true;
                num_f = strtol(optarg, NULL, 0);
                if (num_f <= 0)
                    num_f = INT_MAX;
                break;
            case '?':
                if (optopt == 'g' || optopt == 'p')
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
            default:
                usage(argv[0]);
                return 1;
        }
    }

    int remaining_args = argc - optind;
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        fprintf(stderr, "getcwd() error\n");
        return -1;
    }
    if (remaining_args == 2) {
        strcpy(dir1, cwd);
        strcpy(dir2, cwd);
        strcpy(ext1, argv[optind]);
        strcpy(ext2, argv[optind+1]);
    } else if (remaining_args == 3) {
        strcpy(dir1, argv[optind]);
        strcpy(dir2, argv[optind]);
        strcpy(ext1, argv[optind+1]);
        strcpy(ext2, argv[optind+2]);
    } else if (remaining_args == 4) {
        strcpy(dir1, argv[optind]);
        strcpy(dir2, argv[optind+2]);
        strcpy(ext1, argv[optind+1]);
        strcpy(ext2, argv[optind+3]);
    } else {
        usage(argv[0]);
        return 1;
    }

    // Check for argument format
    if (dir1[strlen(dir1)-1] != '/')
        strcat(dir1, "/");
    if (dir2[strlen(dir2)-1] != '/')
        strcat(dir2, "/");
    if (ext1[0] != '.') {
        char *tempstr = strdup(ext1);
        strcpy(ext1, ".");
        strcat(ext1, tempstr);
    }
    if (ext2[0] != '.') {
        char *tempstr = strdup(ext2);
        strcpy(ext2, ".");
        strcat(ext2, tempstr);
    }

    // Get glob pattern
    char pattern[1024];
    strcpy(pattern, globs);
    strcat(pattern, ext1);

    // Read in input files
    DIR *indir;
    struct dirent *inent;
    indir = opendir(dir1);
    struct stat path_stat;
    int count = 0;

    char ofile[1024];
    int ext1len = strlen(ext1);

    while ((inent = readdir(indir))) {
        // Compare filename to pattern
        if (fnmatch(pattern, inent->d_name, FNM_PATHNAME) == 0) {
            // Now see if the alternate file exists
            char *bn = basename(inent->d_name);
            strcpy(ofile, dir2);
            strncat(ofile, bn, strlen(bn) - ext1len);
            strcat(ofile, ext2);

            if (access(ofile, F_OK) != 0) {
                ++count;
                if (count % 1000 == 0)
                    printf("%d files found so far...\n", count);
                if (print_names) {
                    printf("%s\n", bn);
                    if (count >= num_f) break;
                }
            }
        }
    }
    if (!print_names || count == 0)
        printf("%d %s files found without %s counterparts.\n", count, ext1, ext2);

    return 0;
}

