/**
 * FILE: virtio_cs695_ioctl_common.h
 * --------------------------------
 * Contains definitions of any common compound data-types
 * used by the application-driver pair.
 * Include in user-space and Kernel-Space.
*/

#ifndef __IOCTL_DEFINE_COMMON_H__
#define __IOCTL_DEFINE_COMMON_H__
#include <linux/types.h>

// Multiply via a device.
struct mult_val12_t {
    uint8_t val_1;
    uint8_t val_2;
};

#endif