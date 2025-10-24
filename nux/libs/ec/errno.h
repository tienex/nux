/*
  EC - An embedded non standard C library
  Copyright (C) 2019 Gianluca Guida <glguida@tlbflush.org>

  SPDX-License-Identifier:	BSD-2-Clause
*/

#ifndef EC_ERRNO_H
#define EC_ERRNO_H

#define	EPERM		1	/* Operation not permitted */
#define	ENOENT		2	/* No such device */
#define	ESRCH		3	/* No such process */
#define	EBADF		9	/* Bad file descriptor */
#define	ENOMEM		12	/* Cannot allocate memory */
#define	EACCES		13	/* Permission denied */
#define	EFAULT		14	/* Bad address */
#define	EBUSY		16	/* Device busy */
#define	EEXIST		17	/* Device exists */
#define	ENODEV		19	/* Operation not supported by device */
#define	EINVAL		22	/* Invalid argument */
#define	ENFILE		23	/* Too many open devices in system */
#define	EMFILE		24	/* Too many open devices */
#define	ENOSYS		78	/* Function not implemented */

#endif /* EC_ERRNO_H */
