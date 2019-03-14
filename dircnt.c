#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <fnmatch.h>

void usage(char* prog) {
    fprintf(stderr, "Usage:\n\
        %s dir\n\
        -n: Numeric output only\n\
        -x: Don't print number of files\n\
        -h: Don't count hidden files\n\
        -d: Count directories\n\
        -e: Only find files with the given extension\n\
        -s: Only find files with the given start\n\
        -g: Find files matching the given pattern (incompatible with -e and -s)\n\
        -p: Print n file names. If n <= 0, prints all.\n\
        -f: Print full path to filenames.\n\
        Note: Using any of these options apart from -n disables fast mode.\n", prog);
    return;
}

int main(int argc, char* argv[]) {
    // Argument parsing
    int c;
    bool hidden = true;
    bool ld = false;
    // Extensions
    bool find_ext = false;
    char *ext = NULL;
    int len_ext = 0;
    // Head
    bool find_head = false;
    char *head = NULL;
    int len_head = 0;
    // Glob
    bool find_glob = false;
    char *globs = NULL;
    // Printing
    bool num_output = false;
    bool no_output = false;
    bool print_names = false;
    bool print_full_names = false;
    int num_print = INT_MAX;

    while ((c = getopt(argc, argv, "nhdfxe:s:g:p:")) != -1) {
        switch(c) {
            case 'n':
                num_output = true;
                break;
            case 'h':
                hidden = false;
                break;
            case 'd':
                ld = true;
                break;
            case 'e':
                find_ext = true;
                ext = optarg;
                len_ext = strlen(ext);
                break;
            case 'f':
                print_full_names = true;
                break;
            case 'x':
                no_output = true;
                break;
            case 's':
                find_head = true;
                head = optarg;
                len_head = strlen(head);
                break;
            case 'g':
                find_glob = true;
                globs = optarg;
                break;
            case 'p':
                print_names = true;
                num_print = strtol(optarg, NULL, 0);
                if (num_print <= 0)     num_print = INT_MAX;
                break;
            case '?':
                if (optopt == 'e' || optopt == 's' || optopt == 'p' || optopt == 'g')
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                else
                    fprintf(stderr, "Unknown option '-%c'.\n", optopt);
            default:
                usage(argv[0]);
                return 1;
        }
    }
    if (optind >= argc) {
        fprintf(stderr, "No directory provided.\n");
        usage(argv[0]);
        return 1;
    }

    // File count
    DIR *dir;
    struct dirent *ent;
    long count = 0;
    int len_name = 0;

    dir = opendir(argv[optind]);
    if (ENOENT == errno) {
        fprintf(stderr, "%s is not a valid directory.\n", argv[optind]);
        return -1;
    }

    // Fast mode
    if (hidden && ld && !find_ext && !find_head && !find_glob && !print_names) {
        while((ent = readdir(dir)))
            ++count;
    } else {
        // Slow mode
        chdir(argv[optind]);

        struct stat path_stat;
        while ((ent = readdir(dir))) {
            // Check if file is directory
            if (!ld) {
                if (lstat(ent->d_name, &path_stat) != 0) {
                    fprintf(stderr, "stat failed for %s, error %s\n", ent->d_name, strerror(errno));
                    return -1;
                }
                if (S_ISDIR(path_stat.st_mode))
                    continue;
            }
            // Check if file is hidden
            if (!hidden && ent->d_name[0] == '.')
                continue;
            // Search for glob (takes precedence over prefix or extension
            if (find_glob)
                if (fnmatch(globs, ent->d_name, FNM_PATHNAME) != 0)
                    continue;

            // Search for prefix or extension
            if (find_ext || find_head)
                len_name = strlen(ent->d_name);
            if (find_ext)
                if (len_name < len_ext ||
                        strcmp(&(ent->d_name[len_name-len_ext]), ext) != 0)
                    continue;
            if (find_head)
                if (len_name < len_head ||
                        strncmp(ent->d_name, head, len_head) != 0)
                    continue;
            if (print_names && count < num_print) {
                if (print_full_names)
                    printf("%s/%s\n", argv[optind], ent->d_name);
                else
                    printf("%s\n", ent->d_name);
            }
            ++count;
        }
    }

    closedir(dir);

    if (num_output) printf("%ld\n", count);
    else if (no_output) return 0;
    else {
        printf("%s contains %ld ", argv[optind], count);
        if (!hidden)   printf("non-hidden ");
        if (!ld)       printf("non-directory ");
        printf("files");
        if (find_glob) printf(" matching %s", globs);
        else {
            if (find_head) printf(" starting with %s", head);
            if (find_ext)  printf(" with extension %s", ext);
        }
        printf(".\n");
    }

    return 0;
}
