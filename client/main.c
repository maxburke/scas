/***********************************************************************
 * scas, Copyright (c) 2012-2013, Maximilian Burke
 * This file is distributed under the FreeBSD license. 
 * See LICENSE for details.
 ***********************************************************************/

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>

#define FUSE_USE_VERSION 26
#include <fuse/fuse.h>
#include <sys/errno.h>

#include "scas_base.h"
#include "scas_create.h"
#include "scas_mount.h"

static int
scas_getattr(const char *filename, struct stat *stat_buf)
{
    scas_log("scas_getattr: %s", filename);

    if (strcmp(filename, "/") == 0) 
    {
        stat_buf->st_mode = S_IFDIR | 0755;
        stat_buf->st_nlink = 2;
    }
    else
    {
        return -ENOENT;
    }

    return 0;
}

static int
scas_open(const char *filename, struct fuse_file_info *file_info)
{
    UNUSED(file_info);

    scas_log("scas_open: %s", filename);

    return -1;
}

static int
scas_read(const char *filename, char *buffer, size_t size, off_t offset, struct fuse_file_info *file_info)
{
    UNUSED(filename);
    UNUSED(buffer);
    UNUSED(size);
    UNUSED(offset);
    UNUSED(file_info);

    scas_log("scas_read: %s", filename);
    return -EPERM;
}

static int
scas_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *file_info)
{
    UNUSED(path);
    UNUSED(buf);
    UNUSED(filler);
    UNUSED(offset);
    UNUSED(file_info);

    scas_log("scas_readdir: %s", path);

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    return 0;
}

static void *
scas_init(struct fuse_conn_info *connection)
{
    UNUSED(connection);

    scas_log_init();

    return (void *)1;
}

static void
scas_destroy(void *context)
{
    UNUSED(context);

    scas_log_shutdown();
}

int main(int argc, char *argv[])
{
    struct fuse_operations fuse_ops;
    int i;
    int create_snapshot;
    const char *snapshot;
    const char *snapshot_server;
    const char *snapshot_root;

    memset(&fuse_ops, 0, sizeof(fuse_ops));
    create_snapshot = 0;
    snapshot = NULL;
    snapshot_server = NULL;
    snapshot_root = NULL;

    /*
     * TODO: 
     * Design thoughts:
     * Snapshots should only be created from mounted volumes. This way the
     * knows what changes have been made without resorting to OS tricks like
     * NTFS journals, etc. 
     *
     * The workflow in this case would be to mount a snapshot for revision X,
     * apply changes (ie: sync from the VCS), and then create the new snapshot.
     * SCAS might want to add another operation, reset, to reset the current
     * state to the canonical version of the current revision to facilitate 
     * creating a clean snapshot.
     *
     * Another operation that would come out from this is "populate" which
     * would create a snapshot from an arbitrary volume. Populate would be used
     * to create the initial snapshot, or other initial/arbitrary snapshots.
     */

    for (i = 1; i < (argc - 1); ++i)
    {
        if (i < (argc - 2))
        {
            if (strcmp(argv[i], "-create") == 0)
            {
                create_snapshot = 1;
                snapshot = argv[++i];
                snapshot_root = argv[++i];
                continue;
            }
        }

        if (i < (argc - 1))
        {
            if (strcmp(argv[i], "-snapshot") == 0)
            {
                snapshot = argv[++i];
                continue;
            } 
            else if (strcmp(argv[i], "-server") == 0)
            {
                snapshot_server = argv[++i];
                continue;
            }
        }
    }

    if (create_snapshot)
    {
        return scas_create_snapshot(snapshot, snapshot_root, snapshot_server);
    }

    if (scas_mount_snapshot(snapshot_server, snapshot) != 0)
    {
        return -1;
    }

    fuse_ops.getattr = scas_getattr;
    fuse_ops.open = scas_open;
    fuse_ops.read = scas_read;
    fuse_ops.readdir = scas_readdir;
    fuse_ops.init = scas_init;
    fuse_ops.destroy = scas_destroy;

    return fuse_main(argc, argv, &fuse_ops, NULL);
}

#if 0

static int
scas_readlink(const char *filename, char *buf, size_t buf_size)
{
    return EPERM;
}

static int
scas_getdir(const char *unused0, fuse_dirh_t unused1, fuse_dirfil_t unused2)
{
    UNUSED(unused0);
    UNUSED(unused1);
    UNUSED(unused2);

    return EPERM;
}

static int
scas_mknod(const char *filename, mode_t mode, dev_t dev)
{
    return EPERM;
}

static int
scas_mkdir(const char *directory_name, mode_t mode)
{
    return EPERM;
}

static int
scas_unlink(const char *filename)
{
    return EPERM;
}

static int
scas_rmdir(const char *dir_name)
{
    return EPERM;
}

static int
scas_symlink(const char *old_path, const char *new_path)
{
    return EPERM;
}

static int
scas_rename(const char *old_path, const char *new_path)
{
    return EPERM;
}

static int
scas_link(const char *old_path, const char *new_path)
{
    return EPERM;
}

static int
scas_chmod(const char *filename, mode_t mode)
{
    return EPERM;
}

static int
scas_chown(const char *filename, uid_t uid, gid_t gid)
{
    return EPERM;
}

static int
scas_truncate(const char *filename, off_t length)
{
    return EPERM;
}

static int
scas_utime(const char *filename, struct utimbuf *time_buf)
{
    return EPERM;
}

static int
scas_write(const char *filename, const char *buffer, size_t size, off_t offset, struct fuse_file_info *file_info)
{
    return EPERM;
}

static int
scas_statfs(const char *filename, struct statvfs *stat_buf)
{
    return EPERM;
}

static int
scas_flush(const char *filename, struct fuse_file_info *file_info)
{
    return EPERM;
}

static int
scas_release(const char *filename, struct fuse_file_info *file_info)
{
    return EPERM;
}

static int
scas_fsync(const char *filename, int datasync, struct fuse_file_info *file_info)
{
    return EPERM;
}

#ifdef __APPLE__
static int
scas_setxattr(const char *path, const char *name, const char *value, size_t size, int flags, uint32_t position)
{
    return EPERM;
}

static int
scas_getxattr(const char *path, const char *name, char *value, size_t size, uint32_t position)
{
    return EPERM;
}
#else
static int
scas_setxattr(const char *path, const char *name, const char *value, size_t size, int flags)
{
    return EPERM;
}

static int
scas_getxattr(const char *path, const char *name, char *value, size_t size)
{
    return EPERM;
}
#endif

static int
scas_listxattr(const char *path, char *list, size_t size)
{
    return EPERM;
}

static int
scas_removexattr(const char *path, const char *name)
{
    return EPERM;
}

static int
scas_opendir(const char *path, struct fuse_file_info *file_info)
{
    return EPERM;
}

static int
scas_releasedir(const char *path, struct fuse_file_info *fi)
{
    return EPERM;
}

static int
scas_fsyncdir(const char *path, int datasync, struct fuse_file_info *file_info)
{
    return EPERM;
}

static int
scas_access(const char *path, int mask)
{
    return EPERM;
}

static int
scas_create(const char *path, mode_t mode, struct fuse_file_info *file_info)
{
    return EPERM;
}

static int
scas_ftruncate(const char *path, off_t size, struct fuse_file_info *file_info)
{
    return EPERM;
}

static int
scas_fgetattr(const char *path, struct stat *buf, struct fuse_file_info *file_info)
{
    return EPERM;
}

static int
scas_lock(const char *path, struct fuse_file_info *file_info, int operation, struct flock *lock)
{
    return EPERM;
}

static int
scas_utimens(const char *path, const struct timespec ts[2])
{
    return EPERM;
}

static int
scas_bmap(const char *path, size_t blocksize, uint64_t *index)
{
    return EPERM;
}

int
main(int argc, char **argv)
{
    struct fuse_operations fuse_ops;

    UNUSED(argc);
    UNUSED(argv);

    memset(&fuse_ops, 0, sizeof(struct fuse_operations));
    
    fuse_ops.getattr = scas_getattr;
    fuse_ops.readlink = scas_readlink;
    fuse_ops.getdir = scas_getdir;
    fuse_ops.mknod = scas_mknod;
    fuse_ops.mkdir = scas_mkdir;
    fuse_ops.unlink = scas_unlink;
    fuse_ops.rmdir = scas_rmdir;
    fuse_ops.symlink = scas_symlink;
    fuse_ops.rename = scas_rename;
    fuse_ops.link = scas_link;
    fuse_ops.chmod = scas_chmod;
    fuse_ops.chown = scas_chown;
    fuse_ops.truncate = scas_truncate;
    fuse_ops.utime = scas_utime;
    fuse_ops.open = scas_open;
    fuse_ops.read = scas_read;
    fuse_ops.write = scas_write;
    fuse_ops.statfs = scas_statfs;
    fuse_ops.flush = scas_flush;
    fuse_ops.release = scas_release;
    fuse_ops.fsync = scas_fsync;
    fuse_ops.setxattr = scas_setxattr;
    fuse_ops.getxattr = scas_getxattr;
    fuse_ops.listxattr = scas_listxattr;
    fuse_ops.removexattr = scas_removexattr;
    fuse_ops.opendir = scas_opendir;
    fuse_ops.readdir = scas_readdir;
    fuse_ops.releasedir = scas_releasedir;
    fuse_ops.fsyncdir = scas_fsyncdir;
    fuse_ops.init = scas_init;
    fuse_ops.destroy = scas_destroy;
    fuse_ops.access = scas_access;
    fuse_ops.create = scas_create;
    fuse_ops.ftruncate = scas_ftruncate;
    fuse_ops.fgetattr = scas_fgetattr;
    fuse_ops.lock = scas_lock;
    fuse_ops.utimens = scas_utimens;
    fuse_ops.bmap = scas_bmap;

    return 0;
}

#endif


