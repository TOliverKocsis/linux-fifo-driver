/* SPDX-License-Identifier: GPL-2.0 */
#ifndef TESTFIFO_IOCTL_H
#define TESTFIFO_IOCTL_H

#include <linux/ioctl.h>

/*
 * Magic number: uniquely identifies this driver's ioctl commands.
 * Randomply choosen F for fifo, ignoring possible recomendations by
 * ioctl-number.srt
 */
#define TESTFIFO_IOC_MAGIC 'F'

/*
 * _IO   (magic, nr)            — no argument
 * _IOR  (magic, nr, type)      — read from users point of view
 * _IOW  (magic, nr, type)      — write from users point of view
 *
 * The macro encodes magic + nr + direction + size into a single u32
 * so the kernel can sanity-check commands before dispatching them.
 */
#define FIFO_IOCTL_FLUSH                                                       \
	_IO(TESTFIFO_IOC_MAGIC, 0) /* discard all buffered data */
#define FIFO_IOCTL_GET_COUNT                                                   \
	_IOR(TESTFIFO_IOC_MAGIC, 1,                                            \
	     size_t) /* return bytes currently in buffer */
#define FIFO_IOCTL_GET_SIZE                                                    \
	_IOR(TESTFIFO_IOC_MAGIC, 2, size_t) /* return total buffer capacity */

#endif /* TESTFIFO_IOCTL_H */
