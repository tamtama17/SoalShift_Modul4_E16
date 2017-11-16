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

int parse (char* perms) {
    int bits = 0;
    for (int i = 0; i < 9; i++)
        if (perms[i] != '-')
            bits |= (1 << (8-i));
    return bits;
}

static int do_getattr (const char *path, struct stat *stbuf) {
    int res;

    res = lstat (path, stbuf);
    if (res == -1) return -errno;
    return 0;
}

static int do_readdir (const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    DIR *dp;
    struct dirent *de;

    (void) offset;
    (void) fi;

    dp = opendir (path);
    if (dp == NULL)
        return -errno;

    while ((de = readdir (dp)) != NULL) {
        struct stat st;
        memset (&st, 0, sizeof (st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        if (filler (buf, de->d_name, &st, 0))
            break;
    }

    closedir (dp);
    return 0;
}

static int do_read (const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    if (strstr (path, "Documents") && isForbiddenExtension (path)) {
        // menampilkan pesan error.
        system ("notify-send 'Terjadi kesalahan! File berisi konten berbahaya.'");

        // mengubah nama file menjadi '<namafile>.<ekstensi>.ditandai'.
        char ditandai[1024];
        sprintf (ditandai, "%s.ditandai", path);
        rename (path, ditandai);

        // menyiapkan folder rahasia.
        char rahasiaDir[1024];
        sprintf (rahasiaDir, "%srahasia", getFileDir (path));
        DIR *rahasia = opendir (rahasiaDir);
        if (!rahasia)
            // folder rahasia belum ada. Buat dulu.
            mkdir (rahasiaDir, 0700);

        // memindahkan file ke folder rahasia.
        char rahasiaName[1024];
        sprintf (rahasiaName, "%s/%s", rahasiaDir, getFileName (ditandai));
        rename (ditandai, rahasiaName);

        // men-set file permission menjadi '000' (tidak bisa read write execute)
        chmod (rahasiaName, parse ("---------"));
        return -1;
    }
    else {
        // normal implementation
        int fd, res;

        (void) fi;
        fd = open (path, O_RDONLY);
        if (fd == -1)
            return -errno;

        res = pread (fd, buf, size, offset);
        if (res == -1)
            res = -errno;

        close (fd);
        return res;
    }
}

static struct fuse_operations my_fuse = {
    .getattr    = do_getattr,
    .readdir    = do_readdir,
    .read       = do_read,
};

int main (int argc, char **argv) {
    return fuse_main (argc, argv, &my_fuse, NULL);
}
