/**
 * FILE: spockv2p_ioctl_driver.h
 * --------------------------------
 * Contains Driver Public API declarations and helper structs
 * used by the driver module
 */
#ifndef __IOCTL_DEFINE_DRIVER_H__
#define __IOCTL_DEFINE_DRIVER_H__
#include <linux/fcntl.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <asm/atomic.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include "virtio_cs695_ioctl_common.h"

/* Public API */
static int virtio_cs695_open(struct inode *inode, struct file *filp);
static int virtio_cs695_close(struct inode *inode, struct file *filp);
static long virtio_cs695_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

/* Private API */
static void do_multiply_vq(cs695_req_t *req);

#endif