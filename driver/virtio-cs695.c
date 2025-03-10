#include <linux/virtio.h>
#include <linux/virtio_ring.h>
#include <linux/virtio_ids.h>
#include <linux/virtio_config.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include "virtio-cs695.h"
#include <linux/fs.h>
#include "ioctl.h"					   // ioctl numbers
#include "virtio_cs695_ioctl_driver.h" // Driver-Specific header - Contains file operations, device data struct
#include "virtio_cs695_ioctl_common.h" // App-Driver common datatypes: pa_val_t

#define DEVICE_NAME_MISC "virtio_cs695" // name in /dev

/* device private data (one per device) */
struct virtio_cs695
{
	struct virtio_device *vdev;
	struct virtqueue *vq;
	spinlock_t lock; // not sure of purposekmal
};

struct virtio_cs695 *v695 = NULL;

#if 0
static void virtio_cs695_recv_cb(struct virtqueue *vq)
{
	struct virtio_cs695 *v695 = vq->vdev->priv;
	unsigned int len;
	uint16_t *result;

	// Get the processed buffer back from the device
	result = virtqueue_get_buf(vq, &len);
	pr_info("result length: %d\n", len);
	if (!result)
	{
		pr_err("No buffer received from device\n");
		return;
	}
	// Print the result (this is the multiplication result)
	pr_info("Received multiplication result: %u\n", *result);
}
#endif

static void virtio_cs695_recv_cb(struct virtqueue *vq)
{
	char *buf;
	unsigned int len;

	while ((buf = virtqueue_get_buf(vq, &len)) != NULL)
	{
		pr_info("Buf got len: %lu\n", len);
		pr_info("res = %d\n", buf[0]);
		pr_info("res = %d\n", buf[1]);
	}
}

// virtio_notify from QEMU instantly triggers the callback
// in guest.
static int virtio_cs695_open(struct inode *in, struct file *f)
{
	pr_info("virtio-cs695 FE open called...\n");
	return nonseekable_open(in, f);
}

// - Adds to virtqueue, kicks the device
// - Backend handler responds to kick, reads the vq
// - handler computes a new value and adds it to vq
// - kicks and waits for response??
static int do_multiply_vq(unsigned long arg)
{
	struct mult_val12_t *m = (struct mult_val12_t *)arg;

	struct scatterlist sg_out, sg_in;
	uint16_t *res = kzalloc(sizeof(uint16_t), GFP_KERNEL); // To store the received multiplication result
	struct virtqueue *vq = v695->vq;

	// out req
	sg_init_one(&sg_out, m, sizeof(struct mult_val12_t));

	// in resp
	// tbh, output needs to be in a seperate virtqueue
	sg_init_one(&sg_in, res, sizeof(uint16_t));
	pr_info("both sg_init done\n");

	// Add buffer to the virtqueue
	// #out elems in buf (sending data) = 1
	// #in elems in buf (recv data expected) = 1
	struct scatterlist *sgs[2] = {&sg_out, &sg_in};
	if (virtqueue_add_sgs(vq, sgs, 1, 1, m, GFP_KERNEL) < 0)
	{
		pr_err("Failed to add buffer to virtqueue\n");
		return -EAGAIN;
	}
	pr_info("both sgs added\n");
	virtqueue_kick(vq); // notify. This should result in print in qemu.

	// kfree(res);

	return 0;
}

long virtio_cs695_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	switch (cmd)
	{
	case IOCTL_MULTIPLY:
		// Copy value from user to kernel
		struct mult_val12_t *m = kzalloc(sizeof(struct mult_val12_t), GFP_KERNEL);

		if (copy_from_user(m, (struct mult_val12_t *)arg, sizeof(struct mult_val12_t)))
		{
			return -EFAULT;
		}
		pr_info("<%s>: IOCTL_MULTIPLY(%d, %d)\n", DEVICE_NAME_MISC, m->val_1, m->val_2);
		int err = do_multiply_vq((unsigned long)m);
		break;
	}
	return 0;
}

static const struct file_operations virtio_cs695_fops = {
	.owner = THIS_MODULE,
	.open = virtio_cs695_open,
	// .release = virtio_cs695_release,
	.unlocked_ioctl = virtio_cs695_ioctl,
};

static struct miscdevice virtio_cs695_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = DEVICE_NAME_MISC,
	.mode = 0666,
	.fops = &virtio_cs695_fops,
};

static int virtio_cs695_probe(struct virtio_device *vdev)
{
	pr_info("virtio-cs695 FE probing...\n");

	// @priv: private pointer for the driver's use. (virtio.h)
	v695 = kzalloc(sizeof(*v695), GFP_KERNEL);
	if (!v695)
		return -ENOMEM;

	// point to each other.
	// v695 contains vdev
	vdev->priv = v695;
	v695->vdev = vdev;

	// setup ONE MSI-X interrupt line for ONE virtqueue (vp_request_msix_vectors)
	// internal kernel funcs respect this by using nvqs=1.
	// There is likely another function to ~register n kernel-side virtqueues.
	// with their interrupts
	// /proc/interrupts shows the result of driver interrupt registration
	// info stored in irq_domain struct.
	// ----------------------------------
	// The callback function pointed by dev->vq is triggered when
	// the device has consumed the buffers provided by the driver.
	// - More specifically, the trigger will be an interrupt issued by the hypervisor (see vring_interrupt()).
	// Interrupt request handlers are registered for a virtqueue
	// during the virtqueue setup process (transport-specific).
	v695->vq = virtio_find_single_vq(vdev, virtio_cs695_recv_cb, "cs695-input"); // input virtqueue
	if (IS_ERR(v695->vq))
	{
		kfree(v695);
		return PTR_ERR(v695->vq);
	}

	// print virtqueue info
	uint vq_sz = virtqueue_get_vring_size(v695->vq);
	pr_info("vq ring size: %d\n", vq_sz);

	/* from this point on, the device can notify and get callbacks */
	// performs mmio write to device , setting the status to VIRTIO_CONFIG_S_DRIVER_OK
	// i.e. informing the device that the driver is ready to handle interrupts.
	virtio_device_ready(vdev);

	u64 features = vdev->config->get_features(vdev);

	// (44) 0001 0000 0001 0011 0000 0000 0000 0000 0000 0000 0010
	// (44) 0001 0000 1001 0011 0000 0000 0000 0000 0000 0000 0010 #35 set -> IN_ORDER
	pr_info("Virtio features: 0x%llx\n", features);

	if (virtio_has_feature(vdev, VIRTIO_RING_F_EVENT_IDX))
	{
		pr_info("DND notifications possible\n");
	}

	// finally, create the /dev file
	int err = misc_register(&virtio_cs695_miscdev);
	if (err)
	{
		pr_err("virtio-cs695: register misc device failed.\n");
		goto err_reg_misc;
	}
	return 0;

err_reg_misc:
	vdev->config->del_vqs(vdev);
	return err;
}

static void virtio_cs695_remove(struct virtio_device *vdev)
{
	pr_info("Removing virtio-cs695 device...\n");
	struct virtio_cs695 *dev = vdev->priv;

	/*
	 * disable vq interrupts: equivalent to
	 * vdev->config->reset(vdev)
	 */
	virtio_reset_device(vdev);

	/* detach unused buffers */
	virtqueue_detach_unused_buf(dev->vq);

	/* remove virtqueues */
	vdev->config->del_vqs(vdev);

	kfree(dev);
	misc_deregister(&virtio_cs695_miscdev);
}

#define VIRTIO_ID_SIMPLE 42 // same as in qemu backend.
static const struct virtio_device_id id_table[] = {
	{VIRTIO_ID_SIMPLE, VIRTIO_DEV_ANY_ID}, // {VIRTIO_DEVICE_ID, SOME_KERNEL_FLAG}
	{0},
};

#define VIRTIO_F_CS695_TEST 1
#define VIRTIO_F_IN_ORDER 35
static unsigned int features[] = {
	// add features supported by device
	VIRTIO_F_CS695_TEST,
	VIRTIO_F_IN_ORDER};

static struct virtio_driver virtio_cs695_driver = {
	.feature_table = features,
	.feature_table_size = ARRAY_SIZE(features),
	.driver.name = KBUILD_MODNAME,
	.id_table = id_table,
	.probe = virtio_cs695_probe,
	.remove = virtio_cs695_remove,
};

module_virtio_driver(virtio_cs695_driver);

// https://stackoverflow.com/questions/22901282/hard-time-in-understanding-module-device-tableusb-id-table-usage
// https://web.archive.org/web/20141216112516/http://www.linux-mag.com/id/2617/
// Each driver in the code exposes its vendor/device id using MODULE_DEVICE_TABLE
// 1. At compilation time the build process extracts this infomation from all the drivers
// and prepares a device table.
// 2. When you insert the device, the device table is referred by the kernel and
// if an entry is found matching the device/vendor id of the added device,
// then its module is loaded and initialized.
// NOTE: During boot, the pci enumeration has already stored device id
// in kernel structs.
MODULE_DEVICE_TABLE(virtio, id_table);
MODULE_DESCRIPTION("Driver for virtio-simple-pci");
MODULE_LICENSE("GPL");