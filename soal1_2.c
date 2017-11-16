#define FUSE_USE_VERSION 28
#include <fuse.h>
#include <stdio.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>

/* ambil ekstensi file */
const char *getFileExtension (const char *path) {
    const char *ext = &path[strlen (path) - 1];
    while (*ext != '.') ext--;
    return ext + 1;
}

/* ambil nama file */
const char *getFileName (const char *path) {
    const char *filename = &path[strlen (path) - 1];
    while (*filename != '/') filename--;
    return filename + 1;
}

/* ambil directory dari file */
char *getFileDir (const char *path) {
    char *dir = (char*) malloc (1024 * sizeof (const char));
    strcpy (dir, path);
    for (int i = strlen (dir)-1; i >= 0 && dir[i] != '/'; i--)
        dir[i] = 0;
    return dir;
}

/* cek ekstensi yg dilarang */
int isForbiddenExtension (const char *path) {
    const char *ext = getFileExtension (path);
    return !strcmp ("pdf", ext) || !strcmp ("doc", ext) || !strcmp ("txt", ext);
}



int main (int argc, char **argv) {
    return fuse_main (argc, argv, &my_fuse, NULL);
}
