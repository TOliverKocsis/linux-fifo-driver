# testfifo — Linux Kernel Character Device

A loadable kernel module implementing a FIFO character device (`/dev/testfifo`),
built as a structured exercise in kernel module development.

## What it does

Userspace programs can open `/dev/testfifo` and write data in, then read it back
out in order — a classic first-in, first-out pipe backed by a kernel buffer.

## Features

- **Character device registration** — `cdev` + `alloc_chrdev_region`, with automatic
  `/dev` node creation via `class_create`/`device_create`
- **Circular buffer** — fixed 4096-byte kernel buffer allocated with `kmalloc`,
  managed with head/tail/count pointers
- **Read/write file operations** — `copy_from_user` / `copy_to_user` for safe
  crossing of the kernel/userspace boundary
- **Mutex synchronisation** — `mutex_lock_interruptible` protects shared buffer
  state against concurrent access
- **ioctl interface** — query buffer size, query bytes available, flush the buffer
- **Userspace test program** — a C program in `userspace/` that exercises all
  functionality and edge cases end to end

## Building, Usage and Testing

### 1. Build

From the main directory:

```sh
make
cd userspace && make
```

### 2. Test the Kernel Module

#### Start the virtual kernel

```sh
vng --run
```

#### Insert the character device

```sh
sudo insmod testfifo.ko
```

#### Observe kernel logs

```sh
sudo dmesg | tail -5
```

#### Check that the device file exists

```sh
ls -la /dev/testfifo
```

#### Write to the device and read it back

```sh
sudo chmod 666 /dev/testfifo
echo "hello" > /dev/testfifo
cat /dev/testfifo
```

#### Deregister and remove the module

```sh
sudo rmmod testfifo
```

#### Confirm removal in logs and device listing

```sh
sudo dmesg | tail -5
ls -la /dev/testfifo
```

### 3. Run the Userspace Test Program

```sh
vng --run
sudo insmod testfifo.ko
sudo userspace/fifo_userspace_test
```

## Notes
Some comments in the code are intentionally verbose — they reflect design
decisions and learning notes rather than production documentation style.
