// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2025 Calculinux Project
 * Author: Ben Klop
 *
 * Overlayfs ioctl operations
 */

#include <linux/fs.h>
#include <linux/mount.h>
#include <linux/namei.h>
#include <linux/uaccess.h>
#include <uapi/linux/overlayfs.h>
#include "overlayfs.h"

/**
 * ovl_restore_lower_by_path - Restore visibility of a lower layer file
 * @dentry: overlay dentry
 * @pathname: path relative to overlay mount point
 *
 * This function removes a whiteout character device from the upper layer,
 * making the corresponding lower layer file visible again. It also
 * invalidates the dentry cache entry to ensure the change is immediately
 * visible.
 *
 * Returns 0 on success, negative error code on failure.
 */
static int ovl_restore_lower_by_path(struct dentry *dentry,
					const char *pathname)
{
	struct ovl_fs *ofs = OVL_FS(dentry->d_sb);
	struct dentry *overlay_dentry, *upper_dentry, *upper_parent;
	struct inode *upper_dir;
	struct path path;
	const struct cred *old_cred;
	int err;

	/* Must have a writable upper layer */
	if (!ovl_upper_mnt(ofs))
		return -EROFS;

	/* Lookup the overlay path */
	err = kern_path(pathname, 0, &path);
	if (err)
		return err;

	overlay_dentry = path.dentry;

	/* Verify it's in our overlay */
	if (overlay_dentry->d_sb != dentry->d_sb) {
		err = -EINVAL;
		goto out_path_put;
	}

	/* Get the upper dentry */
	upper_dentry = ovl_dentry_upper(overlay_dentry);
	if (!upper_dentry) {
		/* No upper dentry means no whiteout to remove */
		err = -ENOENT;
		goto out_path_put;
	}

	/* Verify it's actually a whiteout */
	if (!ovl_is_whiteout(upper_dentry)) {
		err = -EINVAL;
		goto out_path_put;
	}

	/* Get the parent directory */
	upper_parent = dget_parent(upper_dentry);
	upper_dir = d_inode(upper_parent);

	/* Lock the parent directory */
	inode_lock(upper_dir);

	/* Remove the whiteout with proper credentials */
	old_cred = ovl_override_creds(dentry->d_sb);
	err = vfs_unlink(ovl_upper_mnt_userns(ofs), upper_dir, upper_dentry, NULL);
	revert_creds(old_cred);

	inode_unlock(upper_dir);
	dput(upper_parent);

	if (err)
		goto out_path_put;

	/* Invalidate the dentry to force a fresh lookup */
	d_drop(overlay_dentry);

	pr_debug("restored lower layer file for %s\n", pathname);

out_path_put:
	path_put(&path);
	return err;
}

/**
 * ovl_ioctl - Handle overlay filesystem ioctl commands
 * @file: file pointer
 * @cmd: ioctl command
 * @arg: ioctl argument
 *
 * Currently supports:
 *   OVL_IOC_RESTORE_LOWER - Restore lower layer file visibility
 */
long ovl_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct dentry *dentry = file->f_path.dentry;
	void __user *argp = (void __user *)arg;
	struct ovl_restore_lower_args args;
	char *pathname;
	long err;

	switch (cmd) {
	case OVL_IOC_RESTORE_LOWER:
		/* Copy arguments from userspace */
		if (copy_from_user(&args, argp, sizeof(args)))
			return -EFAULT;

		/* Validate flags (must be 0 for now) */
		if (args.flags != 0)
			return -EINVAL;

		/* Validate path length */
		if (args.path_len == 0 || args.path_len > PATH_MAX)
			return -EINVAL;

		/* Allocate and copy path string */
		pathname = kmalloc(args.path_len + 1, GFP_KERNEL);
		if (!pathname)
			return -ENOMEM;

		if (copy_from_user(pathname, (char __user *)args.path_ptr,
				   args.path_len)) {
			kfree(pathname);
			return -EFAULT;
		}
		pathname[args.path_len] = '\0';

		/* Perform the operation */
		err = ovl_restore_lower_by_path(dentry, pathname);

		kfree(pathname);
		return err;

	default:
		return -ENOTTY;
	}
}
