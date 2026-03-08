CC     = gcc
CFLAGS = -Wall -Wextra -I..

all: fifo_userspace_test

fifo_userspace_test: fifo_userspace_test.c
	$(CC) $(CFLAGS) -o fifo_userspace_test fifo_userspace_test.c


clean:
	rm -f fifo_userspace_test
