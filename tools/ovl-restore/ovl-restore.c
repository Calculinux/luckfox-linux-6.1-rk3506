/* SPDX-License-Identifier: GPL-2.0 */
/*
 * ovl-restore - Restore visibility of overlayfs lower layer files
 *
 * Copyright (C) 2025 Calculinux Project
 * Author: Ben Klop
 *
 * This tool uses the OVL_IOC_RESTORE_LOWER ioctl to remove whiteout
 * files from an overlayfs upper layer, making the corresponding files
 * in the lower layer visible again.
 *
 * Usage:
 *   ovl-restore <overlay-mount-point> <path>...
 *
 * Example:
 *   # Restore a single file
 *   ovl-restore / /usr/bin/foo
 *
 *   # Restore multiple files
 *   ovl-restore / /usr/bin/foo /usr/lib/libbar.so
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/overlayfs.h>

static void usage(const char *progname)
{
	fprintf(stderr, "Usage: %s <overlay-mount-point> <path>...\n", progname);
	fprintf(stderr, "\n");
	fprintf(stderr, "Restore visibility of overlayfs lower layer files.\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Arguments:\n");
	fprintf(stderr, "  overlay-mount-point  Path to the overlay filesystem mount\n");
	fprintf(stderr, "  path                 One or more paths to restore (absolute paths)\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Example:\n");
	fprintf(stderr, "  %s / /usr/bin/foo /usr/lib/libbar.so\n", progname);
	exit(1);
}

static int restore_lower(int fd, const char *path)
{
	struct ovl_restore_lower_args args;
	int ret;

	args.path_ptr = (__u64)(unsigned long)path;
	args.path_len = strlen(path);
	args.flags = 0;

	ret = ioctl(fd, OVL_IOC_RESTORE_LOWER, &args);
	if (ret < 0) {
		perror("OVL_IOC_RESTORE_LOWER");
		return -1;
	}

	return 0;
}

int main(int argc, char **argv)
{
	const char *mount_point;
	int fd, i;
	int errors = 0;

	if (argc < 3) {
		usage(argv[0]);
	}

	mount_point = argv[1];

	/* Open the overlay mount point */
	fd = open(mount_point, O_RDONLY | O_DIRECTORY);
	if (fd < 0) {
		fprintf(stderr, "Error: Cannot open mount point '%s': %s\n",
			mount_point, strerror(errno));
		return 1;
	}

	/* Process each path */
	for (i = 2; i < argc; i++) {
		const char *path = argv[i];

		printf("Restoring: %s\n", path);

		if (restore_lower(fd, path) < 0) {
			fprintf(stderr, "Error: Failed to restore '%s': %s\n",
				path, strerror(errno));
			errors++;
		} else {
			printf("Successfully restored: %s\n", path);
		}
	}

	close(fd);

	if (errors > 0) {
		fprintf(stderr, "\nCompleted with %d error(s)\n", errors);
		return 1;
	}

	printf("\nAll files restored successfully\n");
	return 0;
}
