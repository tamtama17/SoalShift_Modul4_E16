#define FUSE_USE_VERSION 28
#define HAVE_SETXATTR

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef linux
#define _XOPEN_SOURCE 500
#endif

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>

#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

static int do_getattr (const char *path, struct stat *stbuf) {
    int res;
    res = lstat (path, stbuf);
    if (res == -1) return -errno;
    return 0;
}

static int do_access (const char *path, int mask) {
    int res;
    res = access (path, mask);
    if (res == -1) return -errno;
    return 0;
}

static int do_readlink (const char *path, char *buf, size_t size) {
    int res;
    res = readlink (path, buf, size - 1);
    if (res == -1) return -errno;
    buf[res] = '\0';
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

static int do_mknod (const char *path, mode_t mode, dev_t rdev) {
    int res;
    if (S_ISREG (mode)) {
        res = open (path, O_CREAT | O_EXCL | O_WRONLY, mode);
        if (res >= 0)
            res = close (res);
    }
    else if (S_ISFIFO (mode))
        res = mkfifo (path, mode);
    else
        res = mknod (path, mode, rdev);

    if (res == -1) return -errno;
    return 0;
}

static int do_mkdir (const char *path, mode_t mode) {
    int res;
    res = mkdir (path, mode);
    if (res == -1) return -errno;
    return 0;
}

static int do_unlink (const char *path) {
    int res;
    res = unlink (path);
    if (res == -1) return -errno;
    return 0;
}

static int do_rmdir (const char *path) {
    int res;
    res = rmdir (path);
    if (res == -1) return -errno;
    return 0;
}

static int do_symlink (const char *from, const char *to) {
    int res;
    res = symlink (from, to);
    if (res == -1) return -errno;
    return 0;
}

static int do_rename (const char *from, const char *to) {
    int res;
    res = rename (from, to);
    if (res == -1) return -errno;
    return 0;
}

static int do_link (const char *from, const char *to) {
    int res;
    res = link (from, to);
    if (res == -1) return -errno;
    return 0;
}

static int do_chmod (const char *path, mode_t mode) {
    int res;
    res = chmod (path, mode);
    if (res == -1) return -errno;
    return 0;
}

static int do_chown (const char *path, uid_t uid, gid_t gid) {
    int res;
    res = lchown (path, uid, gid);
    if (res == -1) return -errno;
    return 0;
}

static int do_truncate (const char *path, off_t size) {
    int res;
    res = truncate (path, size);
    if (res == -1) return -errno;
    return 0;
}

static int do_utimens (const char *path, const struct timespec ts[2]) {
    int res;
    struct timeval tv[2];

    tv[0].tv_sec = ts[0].tv_sec;
    tv[0].tv_usec = ts[0].tv_nsec / 1000;
    tv[1].tv_sec = ts[1].tv_sec;
    tv[1].tv_usec = ts[1].tv_nsec / 1000;

    res = utimes (path, tv);
    if (res == -1) return -errno;
    return 0;
}

static int do_open (const char *path, struct fuse_file_info *fi) {
    int res;
    res = open (path, fi->flags);
    if (res == -1) return -errno;
    close (res);
    return 0;
}

static int do_statfs (const char *path, struct statvfs *stbuf) {
    int res;
    res = statvfs (path, stbuf);
    if (res == -1) return -errno;
    return 0;
}

static int do_create (const char* path, mode_t mode, struct fuse_file_info* fi) {
    (void) fi;
    int res;
    res = creat (path, mode);
    if (res == -1) return -errno;
    close (res);
    return 0;
}


static int do_release (const char *path, struct fuse_file_info *fi) {
    /* Just a stub.  This method is optional and can safely be left
       unimplemented */
    (void) path;
    (void) fi;
    return 0;
}

static int do_fsync (const char *path, int isdatasync, struct fuse_file_info *fi) {
    /* Just a stub.  This method is optional and can safely be left
       unimplemented */
    (void) path;
    (void) isdatasync;
    (void) fi;
    return 0;
}

#ifdef HAVE_SETXATTR
static int do_setxattr (const char *path, const char *name, const char *value, size_t size, int flags) {
    int res = lsetxattr (path, name, value, size, flags);
    if (res == -1) return -errno;
    return 0;
}

static int do_getxattr (const char *path, const char *name, char *value, size_t size) {
    int res = lgetxattr (path, name, value, size);
    if (res == -1) return -errno;
    return res;
}

static int do_listxattr (const char *path, char *list, size_t size) {
    int res = llistxattr (path, list, size);
    if (res == -1) return -errno;
    return res;
}

static int do_removexattr (const char *path, const char *name) {
    int res = lremovexattr (path, name);
    if (res == -1) return -errno;
    return 0;
}
#endif /* HAVE_SETXATTR */

/********************************* START HERE *********************************/

/* ambil ekstensi file */
#define file_ext(path) strrchr (path, '.') + 1

/* ambil nama file */
#define file_name(path) strrchr (path, '/') + 1

/* ambil directory dari file */
char *file_dir (const char *path) {
    char *dir = (char*) malloc (1024 * sizeof (char));
    strcpy (dir, path);
    *(file_name (dir)) = 0;
    return dir;
}

static int do_read (const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    if (strstr (file_dir (path), "Downloads") && !strcmp (file_ext (path), "copy")) {
        // file berekstensi .copy di dalam folder 'Downloads' tidak boleh dibuka.
        system ("notify-send 'File yang anda buka adalah file hasil salinan. File tidak bisa diubah maupun disalin kembali!'");
        return 0;
    }

    int fd;
    int res;

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

static int do_write (const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    if (strstr (path, "Downloads")) {
        if (!strcmp (file_ext (path), "copy"))
            // file berekstensi copy diabaikan.
            return 0;

        // menyiapkan folder rahasia.
        char simpananDir[1024];
        sprintf (simpananDir, "%ssimpanan", file_dir (path));
        DIR *simpanan = opendir (simpananDir);
        if (!simpanan)
            // folder rahasia belum ada. Buat dulu.
            mkdir (simpananDir, 0700);

        // membuat file baru di folder simpanan. Nama sama dengan asli.
        char simpananName[1024];
        sprintf (simpananName, "%s/%s", simpananDir, file_name (path));
        FILE *file = fopen (simpananName, "w");

        // meng-copy isi file baru ke file simpanan, instead of file asli.
        fprintf (file, "%s", buf);
        fclose (file);

        // mengubah nama file menjadi '<namafile>.<ekstensi>.copy'.
        char copyName[1024];
        sprintf (copyName, "%s.copy", simpananName);
        rename (simpananName, copyName);
        return 0;
    }
    else {
        // normal implementation
        int fd, res;

        (void) fi;
        fd = open (path, O_WRONLY);
        if (fd == -1)
            return -errno;

        res = pwrite (fd, buf, size, offset);
        if (res == -1) res = -errno;
        close (fd);
        return res;
    }
}

static struct fuse_operations my_fuse = {
    .getattr     = do_getattr,
    .access      = do_access,
    .readlink    = do_readlink,
    .readdir     = do_readdir,
    .mknod       = do_mknod,
    .mkdir       = do_mkdir,
    .symlink     = do_symlink,
    .unlink      = do_unlink,
    .rmdir       = do_rmdir,
    .rename      = do_rename,
    .link        = do_link,
    .chmod       = do_chmod,
    .chown       = do_chown,
    .truncate    = do_truncate,
    .utimens     = do_utimens,
    .open        = do_open,
    .read        = do_read,
    .write       = do_write,
    .statfs      = do_statfs,
    .create      = do_create,
    .release     = do_release,
    .fsync       = do_fsync,

#ifdef HAVE_SETXATTR
    .setxattr    = do_setxattr,
    .getxattr    = do_getxattr,
    .listxattr   = do_listxattr,
    .removexattr = do_removexattr,
#endif
};

int main (int argc, char **argv) {
    return fuse_main (argc, argv, &my_fuse, NULL);
}