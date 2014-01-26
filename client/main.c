/***********************************************************************
 * scas, Copyright (c) 2012-2013, Maximilian Burke
 * This file is distributed under the FreeBSD license. 
 * See LICENSE for details.
 ***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>

#define FUSE_USE_VERSION 26
#include <fuse/fuse.h>
#include <sys/errno.h>

#include "scas_base.h"
#include "scas_create.h"
#include "scas_mount.h"
#include "scas_arg_parse.h"

static int force_mount;
static int reset_snapshot;
static int create_snapshot;
static int mount_snapshot;
static const char *create_snapshot_id;
static const char *mount_snapshot_id;

static const char *
scas_strdup(const char *string)
{
    char *ptr;

    ptr = calloc(strlen(string) + 1, 1);
    strcpy(ptr, string);

    return ptr;
}


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

    // unmount image
    // save state?
    //  * state should be saved when it's changed, not here, in case something
    //    bad happens.

    if (create_snapshot_id)
    {
        free((void *)create_snapshot_id);
    }

    if (mount_snapshot_id)
    {
        free((void *)mount_snapshot_id);
    }

    scas_log_shutdown();
}

static void
scas_parse_arg_force(void *context, const struct scas_arg_t *arg, const char *value)
{
    UNUSED(context);
    UNUSED(arg);
    UNUSED(value);

    force_mount = 1;
}

static void
scas_parse_arg_reset(void *context, const struct scas_arg_t *arg, const char *value)
{
    UNUSED(context);
    UNUSED(arg);
    UNUSED(value);

    reset_snapshot = 1;
}

static void
scas_parse_arg_mount(void *context, const struct scas_arg_t *arg, const char *value)
{
    UNUSED(context);
    UNUSED(arg);

    mount_snapshot = 1;
    mount_snapshot_id = scas_strdup(value);
}

static void
scas_parse_arg_create(void *context, const struct scas_arg_t *arg, const char *value)
{
    UNUSED(context);
    UNUSED(arg);

    create_snapshot    = 1;
    create_snapshot_id = scas_strdup(value);
}

static int
scas_is_valid_args(void)
{
    #define VALIDATE(x, str) if (!(x)) { fprintf(stderr, "%s\n", str); return 0; } else (void)0

    if (reset_snapshot)
    {
        VALIDATE(!force_mount, "Force mount cannot be used with new.");
        VALIDATE(!create_snapshot, "Create snapshot cannot be used with new.");
        VALIDATE(!mount_snapshot, "Mount snapshot cannot be used with new.");
        return 0;
    }

    if (create_snapshot)
    {
        VALIDATE(!force_mount, "Force mount cannot be used with create.");
        VALIDATE(!reset_snapshot, "New snapshot cannot be used with create.");
        VALIDATE(!mount_snapshot, "Mount snapshot cannot be used with create.");
        return 0;
    }

    if (mount_snapshot)
    {
        VALIDATE(!force_mount, "Force mount cannot be used with mount.");
        VALIDATE(!reset_snapshot, "New snapshot cannot be used with mount.");
        VALIDATE(!create_snapshot, "Create snapshot cannot be used with mount.");
        return 0;
    }


    #undef VALIDATE

    return 1;
}

static void
scas_parse_args(int argc, char **argv)
{
    struct scas_arg_t args[] = 
    {
        { "-f", "--force",  ARG_TYPE_SWITCH,    scas_parse_arg_force },
        { "-r", "--reset",  ARG_TYPE_SWITCH,    scas_parse_arg_reset },
        { "-m", "--mount",  ARG_TYPE_PARAMETER, scas_parse_arg_mount },
        { "-c", "--create", ARG_TYPE_PARAMETER, scas_parse_arg_create },
    };
    struct scas_arg_context_t context = 
    {
        argc,
        argv,
        NULL,
        sizeof args / sizeof args[0],
        args,
        NULL                        /* No positional callback as positional
                                       args are consumed by FUSE. */
    };

    scas_arg_parse(&context);
}

int main(int argc, char *argv[])
{
    struct fuse_operations fuse_ops;

    memset(&fuse_ops, 0, sizeof(fuse_ops));

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


    /*
     * TODO August 5, 2013
     * typical snapshot workflow:
     *   1a. Mount an existing snapshot: 
     *      scas_client --mount <snapshot ID> /fs/path
     *   1b. or create a new one from zero: 
     *      scas_client --reset /fs/path
     *   2. Make changes. Sync files, etc.
     *   3. Unmount: 
     *      umount /fs/path
     *   4. Create the snapshot: 
     *      scas_client --create <snapshot ID>
     *
     * reset workflow: 
     *   1. Mount an existing snapshot: 
     *      scas_client --mount <snapshot ID> /fs/path 
     *   2. Unmount: 
     *      umount /fs/path
     *   3. Re-mount, or mount a new snapshot:
     *      scas_client --force -mount <snapshot ID> /fs/path
     *
     * If "-force" isn't specified in the reset workflow, the client should warn
     * that a current snapshot exists and will require -force in order to 
     * discard.
     */
    scas_parse_args(argc, argv);
    
    // TODO:
    //  * validate args
    //  * mount image here

    if (!scas_is_valid_args())
    {
        return -1;
    }

    scas_mount_image();

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


