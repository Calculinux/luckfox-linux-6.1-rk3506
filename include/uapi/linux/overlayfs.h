/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 * Overlayfs filesystem user API
 */

#ifndef _UAPI_LINUX_OVERLAYFS_H
#define _UAPI_LINUX_OVERLAYFS_H

#include <linux/types.h>
#include <linux/ioctl.h>

/*
 * OVL_IOC_RESTORE_LOWER - Restore visibility of a lower layer file
 *
 * This ioctl removes a whiteout file from the upper layer, making the
 * corresponding file in the lower layer visible again. This is useful
 * when a lower-layer file has been unintentionally hidden by whiteout
 * creation (e.g., after removing a duplicate package in a layered system).
 *
 * The path should be relative to the overlay mount point.
 *
 * Returns:
 *   0 on success
 *   -ENOENT if no whiteout exists at the specified path
 *   -EINVAL if path is invalid or doesn't point to a whiteout
 *   -EPERM if caller lacks permissions
 *   -EROFS if overlay is read-only
 */
struct ovl_restore_lower_args {
	__aligned_u64 path_ptr;		/* Pointer to path string */
	__u32 path_len;			/* Length of path string */
	__u32 flags;			/* Reserved for future use, must be 0 */
};

/*
 * OVL_IOC_IS_RESTORABLE - Check if a file can be restored
 *
 * This ioctl checks whether a given path has a whiteout in the upper layer
 * that can be removed to restore the lower layer file. This allows userspace
 * to query restorability without actually performing the restoration.
 *
 * Uses the same argument structure as OVL_IOC_RESTORE_LOWER.
 *
 * Returns:
 *   0 if the file is restorable (has a whiteout)
 *   -ENOENT if no whiteout exists at the specified path
 *   -EINVAL if path is invalid
 *   -EROFS if overlay is read-only (no upper layer)
 */
struct ovl_is_restorable_args {
	__aligned_u64 path_ptr;		/* Pointer to path string */
	__u32 path_len;			/* Length of path string */
	__u32 flags;			/* Reserved for future use, must be 0 */
};

#define OVL_IOC_RESTORE_LOWER _IOW('O', 1, struct ovl_restore_lower_args)
#define OVL_IOC_IS_RESTORABLE _IOR('O', 2, struct ovl_is_restorable_args)

#endif /* _UAPI_LINUX_OVERLAYFS_H */
