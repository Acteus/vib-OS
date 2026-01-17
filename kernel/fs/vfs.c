/*
 * UnixOS Kernel - Virtual Filesystem Implementation
 */

#include "fs/vfs.h"
#include "printk.h"

/* ===================================================================== */
/* Static data */
/* ===================================================================== */

/* Registered filesystems */
static struct file_system_type *file_systems = NULL;

/* Mount points */
static struct vfsmount *mounts[MAX_MOUNTS];
static int mount_count = 0;

/* Root filesystem */
static struct vfsmount *root_mount = NULL;
static struct dentry *root_dentry = NULL;

/* ===================================================================== */
/* Helper functions */
/* ===================================================================== */

static struct file_system_type *find_filesystem(const char *name)
{
    struct file_system_type *fs = file_systems;
    while (fs) {
        /* Compare names */
        const char *a = fs->name;
        const char *b = name;
        while (*a && *b && *a == *b) {
            a++;
            b++;
        }
        if (*a == '\0' && *b == '\0') {
            return fs;
        }
        fs = fs->next;
    }
    return NULL;
}

static int path_compare(const char *a, const char *b)
{
    while (*a && *b && *a == *b) {
        a++;
        b++;
    }
    return *a - *b;
}

/* ===================================================================== */
/* VFS initialization */
/* ===================================================================== */

void vfs_init(void)
{
    printk(KERN_INFO "VFS: Initializing virtual filesystem\n");
    
    /* Clear mount table */
    for (int i = 0; i < MAX_MOUNTS; i++) {
        mounts[i] = NULL;
    }
    
    /* TODO: Register built-in filesystems */
    /* register_filesystem(&ramfs_type); */
    /* register_filesystem(&procfs_type); */
    /* register_filesystem(&sysfs_type); */
    /* register_filesystem(&devfs_type); */
    
    printk(KERN_INFO "VFS: Initialized\n");
}

/* ===================================================================== */
/* Filesystem registration */
/* ===================================================================== */

int register_filesystem(struct file_system_type *fs)
{
    if (!fs || !fs->name) {
        return -EINVAL;
    }
    
    /* Check for duplicate */
    if (find_filesystem(fs->name)) {
        printk(KERN_WARNING "VFS: Filesystem '%s' already registered\n", fs->name);
        return -EBUSY;
    }
    
    /* Add to list */
    fs->next = file_systems;
    file_systems = fs;
    
    printk(KERN_INFO "VFS: Registered filesystem '%s'\n", fs->name);
    
    return 0;
}

/* ===================================================================== */
/* File operations */
/* ===================================================================== */

struct file *vfs_open(const char *path, int flags, mode_t mode)
{
    (void)path;
    (void)flags;
    (void)mode;
    
    /* TODO: Implement path lookup and file opening */
    printk(KERN_DEBUG "VFS: open('%s', 0x%x, 0%o)\n", path, flags, mode);
    
    return NULL;  /* Not implemented */
}

int vfs_close(struct file *file)
{
    if (!file) {
        return -EBADF;
    }
    
    /* Call release if defined */
    if (file->f_op && file->f_op->release && file->f_dentry) {
        file->f_op->release(file->f_dentry->d_inode, file);
    }
    
    /* Decrement reference count */
    file->f_count.counter--;
    
    /* TODO: Free file structure if count reaches 0 */
    
    return 0;
}

ssize_t vfs_read(struct file *file, char *buf, size_t count)
{
    if (!file) {
        return -EBADF;
    }
    
    if (!buf) {
        return -EFAULT;
    }
    
    if (!file->f_op || !file->f_op->read) {
        return -EINVAL;
    }
    
    return file->f_op->read(file, buf, count, &file->f_pos);
}

ssize_t vfs_write(struct file *file, const char *buf, size_t count)
{
    if (!file) {
        return -EBADF;
    }
    
    if (!buf) {
        return -EFAULT;
    }
    
    if (!file->f_op || !file->f_op->write) {
        return -EINVAL;
    }
    
    return file->f_op->write(file, buf, count, &file->f_pos);
}

loff_t vfs_lseek(struct file *file, loff_t offset, int whence)
{
    if (!file) {
        return -EBADF;
    }
    
    loff_t new_pos;
    struct inode *inode = file->f_dentry ? file->f_dentry->d_inode : NULL;
    
    switch (whence) {
        case SEEK_SET:
            new_pos = offset;
            break;
        case SEEK_CUR:
            new_pos = file->f_pos + offset;
            break;
        case SEEK_END:
            if (!inode) {
                return -EINVAL;
            }
            new_pos = inode->i_size + offset;
            break;
        default:
            return -EINVAL;
    }
    
    if (new_pos < 0) {
        return -EINVAL;
    }
    
    file->f_pos = new_pos;
    return new_pos;
}

/* ===================================================================== */
/* Directory operations */
/* ===================================================================== */

int vfs_mkdir(const char *path, mode_t mode)
{
    (void)path;
    (void)mode;
    
    printk(KERN_DEBUG "VFS: mkdir('%s', 0%o)\n", path, mode);
    
    /* TODO: Implement */
    return -ENOSYS;
}

int vfs_rmdir(const char *path)
{
    (void)path;
    
    printk(KERN_DEBUG "VFS: rmdir('%s')\n", path);
    
    /* TODO: Implement */
    return -ENOSYS;
}

int vfs_unlink(const char *path)
{
    (void)path;
    
    printk(KERN_DEBUG "VFS: unlink('%s')\n", path);
    
    /* TODO: Implement */
    return -ENOSYS;
}

int vfs_rename(const char *oldpath, const char *newpath)
{
    (void)oldpath;
    (void)newpath;
    
    printk(KERN_DEBUG "VFS: rename('%s', '%s')\n", oldpath, newpath);
    
    /* TODO: Implement */
    return -ENOSYS;
}

/* ===================================================================== */
/* Mount operations */
/* ===================================================================== */

int vfs_mount(const char *source, const char *target, const char *fstype,
              unsigned long flags, const void *data)
{
    (void)flags;
    (void)data;
    
    printk(KERN_INFO "VFS: mount('%s', '%s', '%s')\n", source, target, fstype);
    
    /* Find filesystem type */
    struct file_system_type *fs = find_filesystem(fstype);
    if (!fs) {
        printk(KERN_ERR "VFS: Unknown filesystem type '%s'\n", fstype);
        return -ENODEV;
    }
    
    /* Check mount limit */
    if (mount_count >= MAX_MOUNTS) {
        return -ENOMEM;
    }
    
    /* Call filesystem's mount function */
    if (!fs->mount) {
        return -ENOSYS;
    }
    
    struct super_block *sb = fs->mount(fs, flags, source, (void *)data);
    if (!sb) {
        return -EIO;
    }
    
    /* Create mount structure */
    /* TODO: Allocate properly */
    static struct vfsmount mount_pool[MAX_MOUNTS];
    struct vfsmount *mnt = &mount_pool[mount_count];
    
    mnt->mnt_root = sb->s_root;
    mnt->mnt_sb = sb;
    mnt->mnt_mountpoint = NULL;  /* TODO: Find target dentry */
    mnt->mnt_parent = root_mount;
    
    /* Copy device name */
    int i;
    for (i = 0; i < 63 && source[i]; i++) {
        mnt->mnt_devname[i] = source[i];
    }
    mnt->mnt_devname[i] = '\0';
    
    mounts[mount_count++] = mnt;
    
    /* If mounting root, set root_mount */
    if (path_compare(target, "/") == 0) {
        root_mount = mnt;
        root_dentry = sb->s_root;
    }
    
    printk(KERN_INFO "VFS: Mounted '%s' on '%s'\n", source, target);
    
    return 0;
}

int vfs_umount(const char *target)
{
    printk(KERN_INFO "VFS: umount('%s')\n", target);
    
    /* Find mount point */
    for (int i = 0; i < mount_count; i++) {
        if (mounts[i] && mounts[i]->mnt_root) {
            /* TODO: Compare mount point */
            /* For now, just mark as unmounted */
        }
    }
    
    return -ENOSYS;
}
