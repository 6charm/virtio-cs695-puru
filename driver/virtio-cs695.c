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
#include <linux/completion.h>

#define DEVICE_NAME_MISC "virtio_cs695" // name in /dev

/* device private data (one per device) */
struct virtio_cs695
{
	struct virtio_device *vdev;
	struct virtqueue *vq;
	uint16_t mult_res;
	struct completion req_done;
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


static void virtio_cs695_recv_cb(struct virtqueue *vq)
{
	uint16_t *buf;
	unsigned int len;

	while ((buf = virtqueue_get_buf(vq, &len)) != NULL)
	{
		pr_info("Buf got len: %lu\n", len);
		pr_info("res = %d\n", *buf);
	}
}
#endif

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
static void
do_multiply_vq(cs695_req_t *req)
{

	struct scatterlist sg_out, sg_in, *sgs[2]; // outbuf, inbuf

	sg_init_one(&sg_out, &(req->m), sizeof(req->m));
	sgs[0] = &sg_out;

	sg_init_one(&sg_in, &(req->res), sizeof(req->res));
	sgs[1] = &sg_in;

	// Key: out_sgs must be before the in_sgs in the sgs list.
	// see virtqueue_add_sgs() ->  virtqueue_add_split() in linux kernel.
	virtqueue_add_sgs(v695->vq, sgs, 1, 1, req, GFP_ATOMIC);

	// only kick the outbuf vq
	virtqueue_kick(v695->vq); // calls cs695_read_outbuf() in qemu

	// wait for response
	// int len;
	// while (virtqueue_get_buf(v695->vq, &len) == NULL)
	// 	cpu_relax();
}

long virtio_cs695_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	switch (cmd)
	{
	case IOCTL_MULTIPLY:
		// Copy value from user to kernel
		multiplicands_t m;

		if (copy_from_user(&m, (struct mult_val12_t *)arg, sizeof(m)))
		{
			return -EFAULT;
		}
		pr_info("<%s>: IOCTL_MULTIPLY(%d, %d)\n", DEVICE_NAME_MISC, m.val_1, m.val_2);

		// create the req
		cs695_req_t *req = kzalloc(sizeof(cs695_req_t), GFP_KERNEL);
		req->m = m;
		init_completion(&(v695->req_done));

		do_multiply_vq(req); // becomes asynchronous with the callback
		pr_info("Waiting for multiplication result...\n");
		pr_info("Multiply result: %d\n", req->res);

		wait_for_completion(&(v695->req_done));

		// return mult result to user from the v695 device
		copy_to_user(&((cs695_req_t *)arg)->res, &(v695->mult_res), sizeof(v695->mult_res));

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

#if 1
// reads device-written data from inbufs
static void virtio_cs695_inbufs_cb(struct virtqueue *vq)
{
	pr_info("cb reached\n");
	cs695_req_t *req;
	int len;
	if ((req = virtqueue_get_buf(vq, &len)) != NULL)
	{
		pr_info("Response received in callback");
		v695->mult_res = req->res;
		complete(&(v695->req_done));
	}
}
#endif

// device never writes to outbuf vq.
// So the only purpose here is to remove the
// descriptor from the vq.
#if 0
static void virtio_cs695_outbufs_cb(struct virtqueue *vq)
{
	pr_info("outbufs cb called\n");
	int len;
	void *buf = virtqueue_get_buf(vq, &len); // this will be the original outbuf
	/* free sent data */
	if (buf)
	{
		kfree(buf);
	}
	return;
}
#endif

#if 0
static int virtio_cs695_assign_virtqueues(void)
{
	const char *names[] = {"cs695-outbufs", "cs695-inbufs"};
	vq_callback_t *callbacks[] = {virtio_cs695_outbufs_cb, virtio_cs695_inbufs_cb};
	struct virtqueue *vqs[2];

	int err = virtio_find_vqs(v695->vdev, 2, vqs, callbacks, names, NULL);
	if (err)
	{
		return err;
	}
	v695->tx_vq = vqs[0];
	v695->rx_vq = vqs[1];
	return 0;
}
#endif

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

#if 1
	v695->vq = virtio_find_single_vq(vdev, virtio_cs695_inbufs_cb, "cs695-bidirectional"); // output virtqueue
#endif

	// int err1 = virtio_cs695_assign_virtqueues();

	/* from this point on, the device can notify and get callbacks */
	// performs mmio write to device , setting the status to VIRTIO_CONFIG_S_DRIVER_OK
	// i.e. informing the device that the driver is ready to handle interrupts.
	virtio_device_ready(vdev);

	u64 features = vdev->config->get_features(vdev);

	// (44) 0001 0000 0001 0011 0000 0000 0000 0000 0000 0000 0010
	// (44) 0001 0000 1001 0011 0000 0000 0000 0000 0000 0000 0010 #35 set -> IN_ORDER
	pr_info("Virtio features: 0x%llx\n", features);

	// if (virtio_has_feature(vdev, VIRTIO_RING_F_EVENT_IDX))
	// {
	// 	pr_info("DND notifications possible\n");
	// }

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