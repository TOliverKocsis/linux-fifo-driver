// SPDX-License-Identifier: GPL-2.0
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>

#include "../testfifo_ioctl.h"

#define DEVICE "/dev/testfifo"

int main(void)
{
	printf("Starting userspace testing\n");

	int fd = open(DEVICE, O_RDWR);
	if (fd < 0) {
		perror("Open fifo device failed");
		return EXIT_FAILURE;
	}

	// --- write/read round-trip ---
	char msg[] = "hello fifo";
	ssize_t written = write(fd, msg, strlen(msg));
	if (written < 0) {
		perror("Write to fifo device failed");
		close(fd);
		return EXIT_FAILURE;
	}
	printf("Wrote %zd bytes: '%s'\n", written, msg);

	char read_back[sizeof(msg)];
	ssize_t read_bytes = read(fd, read_back, strlen(msg));
	if (read_bytes < 0) {
		perror("Read from fifo failed");
		close(fd);
		return EXIT_FAILURE;
	}
	read_back[read_bytes] = '\0'; // read() does not null-terminate
	printf("Read %zd bytes: '%s'\n", read_bytes, read_back);
	if (strcmp(msg, read_back) == 0) {
		printf("Round-trip: PASS\n");
	} else {
		printf("Round-trip: FAIL (expected '%s', got '%s')\n", msg,
		       read_back);
	}

	// --- FIFO_IOCTL_GET_SIZE ---
	size_t buf_size = 0;
	if (ioctl(fd, FIFO_IOCTL_GET_SIZE, &buf_size) < 0) {
		perror("ioctl GET_SIZE failed");
		close(fd);
		return EXIT_FAILURE;
	}
	printf("Buffer size: %zu bytes\n", buf_size);

	// --- FIFO_IOCTL_GET_COUNT ---
	// buffer was fully drained by read above, so count should be 0
	size_t buf_count = 0;
	if (ioctl(fd, FIFO_IOCTL_GET_COUNT, &buf_count) < 0) {
		perror("ioctl GET_COUNT failed");
		close(fd);
		return EXIT_FAILURE;
	}
	printf("Bytes in buffer after read: %zu\n", buf_count);

	// --- FIFO_IOCTL_FLUSH ---
	// write something in so there is data to flush
	write(fd, msg, strlen(msg));
	ioctl(fd, FIFO_IOCTL_GET_COUNT, &buf_count);
	printf("Bytes in buffer before flush: %zu\n", buf_count);

	if (ioctl(fd, FIFO_IOCTL_FLUSH) < 0) {
		perror("ioctl FLUSH failed");
		close(fd);
		return EXIT_FAILURE;
	}

	ioctl(fd, FIFO_IOCTL_GET_COUNT, &buf_count);
	printf("Bytes in buffer after flush: %zu\n", buf_count);

	// --- edge case: read from empty buffer ---
	char tmp[16];
	ssize_t empty_read = read(fd, tmp, sizeof(tmp));
	if (empty_read == 0) {
		printf("Read from empty buffer: PASS (returned 0)\n");
	} else {
		printf("Read from empty buffer: FAIL (returned %zd)\n",
		       empty_read);
	}

	// --- edge case: write more than buffer size ---
	// buf_size bytes already fill it, one extra byte should be rejected
	char *big = malloc(buf_size + 1);
	if (big) {
		memset(big, 'A', buf_size + 1);
		ioctl(fd, FIFO_IOCTL_FLUSH);
		write(fd, big, buf_size);
		ssize_t overflow = write(fd, "X", 1);
		if (overflow < 0)
			printf("Write to full buffer: PASS (rejected with "
			       "error)\n");
		else
			printf(
			    "Write to full buffer: FAIL (accepted %zd bytes)\n",
			    overflow);
		free(big);
		ioctl(fd, FIFO_IOCTL_FLUSH);
	}

	// --- edge case: verify GET_COUNT is 0 after flush ---
	ioctl(fd, FIFO_IOCTL_GET_COUNT, &buf_count);
	if (buf_count == 0) {
		printf("GET_COUNT after flush: PASS\n");
	} else {
		printf("GET_COUNT after flush: FAIL (got %zu)\n", buf_count);
	}

	close(fd);
	return EXIT_SUCCESS;
}
