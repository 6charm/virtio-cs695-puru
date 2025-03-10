#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <error.h>
#include <assert.h>
#include <string.h>
#include <sys/ioctl.h>

#include "../driver/virtio_cs695_ioctl_common.h"
#include "../driver/ioctl.h" // Double include of common.h handled by header guard.

#define DEVICE_FILE_PATH "/dev/virtio_cs695"

/* Error Codes */
static const int e_FailedDeviceOpen = 1;
static const int e_FailedDeviceClose = 2;
static const int e_FailedIoctlCommandV2P = 4;
// static const int e_FailedIoctlCommandSetValue = 8;
// static const int e_FailedTestSetValue = 9;
// static const int e_FailedIoctlCommandGetValue = 16;

static const int e_FailedMalloc = 32;

static void *safe_malloc(size_t sz)
{
    void *ptr = malloc(sz);
    if (!ptr)
    {
        error(e_FailedMalloc, errno, "Malloc of size %ld failed.\n", sz);
        exit(EXIT_FAILURE);
    }
    memset(ptr, 0, sz);
    return ptr;
}

static int open_device(const char *driver_module_name)
{
    printf("** Open Device **\n");

    int driver_fd = open(driver_module_name, O_RDWR); // How to decide flag ?
    if (driver_fd == -1)
    {
        error(e_FailedDeviceOpen, errno, "Could not open \"%s\".\n", driver_module_name);
        exit(EXIT_FAILURE);
    }
    return driver_fd;
}

static void close_device(const char *driver_module_name, int driver_fd)
{
    printf("** Close Device **\n");

    int ret = close(driver_fd);
    if (ret == -1)
    {
        error(e_FailedDeviceClose, errno, "Could not close \"%s\".\n", driver_module_name);
        exit(EXIT_FAILURE);
    }
}

int main(void)
{
    // struct containing multiplicands
    struct mult_val12_t *m = safe_malloc(sizeof(struct mult_val12_t));
    m->val_1 = 5;
    m->val_2 = 7;

    int ioctl_device_fd = open_device(DEVICE_FILE_PATH);

    if (ioctl(ioctl_device_fd, IOCTL_MULTIPLY, m) < 0)
    {
        error(e_FailedIoctlCommandV2P, errno, "IOCTL 0 failed.\n");
        exit(EXIT_FAILURE);
    }

    close_device(DEVICE_FILE_PATH, ioctl_device_fd);

    free(m);

    return EXIT_SUCCESS;
}