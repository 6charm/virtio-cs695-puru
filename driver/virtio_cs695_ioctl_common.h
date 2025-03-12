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
typedef struct multiplicands
{
    uint8_t val_1;
    uint8_t val_2;
} multiplicands_t;

typedef uint16_t prod_t;

typedef struct cs695_req
{
    multiplicands_t m;
    prod_t res;
} cs695_req_t;

#endif