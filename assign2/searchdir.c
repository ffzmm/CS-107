/*
 * File: searchdir.c
 * Author: Iris Yang 
 * ----------------------
 */

#include "cmap.h"
#include "cvector.h"
#include <dirent.h>
#include <error.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define NFILES_ESTIMATE 20

/* sort_string is a callback function that compares string a and string b. Returns negative if a is shorter than
   b, or the lexcicographically before b for the same length; returns negative elsewise
*/ 
int sort_string_cmp(const void *a, const void *b) {
    char ** st1 = (char**) a;
    char ** st2 = (char**) b;
    if (strlen(*st1) < strlen(*st2) ) {
        return -1;
    } else if (strlen(*st1) == strlen(*st2)) {
        return (strcmp(*st1, *st2));
    } else
        return 1;
}

/* sort_int_cmp: callback function that compares two integers pointed by a and b.
   Returns postive if (*a) > (*b), and negative if (*a) < (*b); returns 0 if they are equal.
*/
int sort_int_cmp(const void *a, const void *b) {
    int * int1 = (int*) a;
    int * int2 = (int*) b;
    return (*int1) - (*int2);
}

static void gather_files(CVector *matches, CVector *visited, const char *searchstr, const char *dirname)
{
    DIR *dp = opendir(dirname); 
    struct dirent *entry;
    while (dp != NULL && (entry = readdir(dp)) != NULL) { /* iterate over entries in dir */
        if (entry->d_name[0] == '.') continue; /* skip hidden files */

        char fullpath[PATH_MAX];
        sprintf(fullpath, "%s/%s", dirname, entry->d_name); /* construct full path */

        struct stat ss;
        if (stat(fullpath, &ss) == 0 && S_ISDIR(ss.st_mode)) {  /* if subdirectory, recur */
            int inode = ss.st_ino;    /* inode number is unique id per entry in filesystem */
            if (cvec_search(visited, (int*) &inode, sort_int_cmp, 0, 1) == -1) {; // sorted           
                cvec_append(visited, &inode);
                gather_files(matches, visited, searchstr, fullpath); // recur only if inode is not visited
            }
        } else {
            if ( strstr(fullpath, searchstr) != NULL) {
                char* toAppend = strdup(fullpath);
                cvec_append(matches, &toAppend );      /* else add to vector */
            }
        }
    }
    closedir(dp);
}

int main(int argc, char *argv[])
{
    if (argc < 2) error(1,0, "you must specify the search string and optionally the diectory to search.");
    char *searchstr = argv[1];
    char *dirname = argc < 3 ? "." : argv[2]; /* use this dir as default if no directory specified */
    if (access(dirname, R_OK) == -1) error(1,0, "cannot access path \"%s\"", dirname);

    CVector *matches = cvec_create(sizeof(char*), NFILES_ESTIMATE, NULL);
    CVector *visited = cvec_create(sizeof(int), NFILES_ESTIMATE, NULL);
    gather_files(matches, visited, searchstr, dirname);
    cvec_sort(matches, sort_string_cmp);
    for (char **cur = cvec_first(matches); cur != NULL; cur = cvec_next(matches, cur)) {
        printf("%s\n", *cur);
        free(*cur);
    }
    cvec_dispose(matches);
    return 0;
}

