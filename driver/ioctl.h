/**
 * FILE: ioctl.h
 * --------------
 * Contains the #defined ioctl numbers supported by
 * the driver. Include this file in the kernel module
 * and user-space application. Header guards are named
 * to prevent clashes with kernel ioctl.h
*/
#ifndef __IOCTL_DEFINE_H__
#define __IOCTL_DEFINE_H__
#include "virtio_cs695_ioctl_common.h" // struct gva_val_t

#define IOCTL_MAGIC 'W'

/* Create IOCTL numbers for 3 userspace request-types */
#define IOCTL_MULTIPLY            _IOWR(IOCTL_MAGIC, 0, struct mult_val12_t)              // multiply two numbers

#endif