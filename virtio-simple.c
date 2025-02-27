#include <linux/virtio.h>
#include <linux/virtio_ids.h>
#include <linux/virtio_config.h>
#include <linux/module.h>

/* device private data (one per device) */
struct virtio_simple_dev {
        struct virtqueue *vq;
};

static void virtio_simple_recv_cb(struct virtqueue *vq)
{
        struct virtio_simple_dev *dev = vq->vdev->priv;
        char *buf;
        unsigned int len;

        while ((buf = virtqueue_get_buf(dev->vq, &len)) != NULL) {
                pr_info("%c\n", buf[0]);
        }
}

static int virtio_simple_probe(struct virtio_device *vdev)
{
        struct virtio_simple_dev *dev = NULL;
        pr_info("virtio-simple FE probing...\n");

        /* initialize device data */
        dev = kzalloc(sizeof(struct virtio_simple_dev), GFP_KERNEL);
        if (!dev)
                return -ENOMEM;

        // setup ONE MSI-X interrupt line for ONE virtqueue (vp_request_msix_vectors)
        // internal kernel funcs respect this by using 1 
        // as nvqs.
        // There is likely another function to ~register n kernel-side virtqueues.
        // with their interrupts
        // /proc/interrupts shows the result of driver interrupt registration
        // info stored in irq_domain struct.
        dev->vq = virtio_find_single_vq(vdev, virtio_simple_recv_cb, "input"); // input virtqueue
        if (IS_ERR(dev->vq)) {
                kfree(dev);
                return PTR_ERR(dev->vq);

        }
        vdev->priv = dev;

        /* from this point on, the device can notify and get callbacks */
        // performs mmio write to device , setting the status to VIRTIO_CONFIG_S_DRIVER_OK
        // i.e. informing the device that the driver is ready to handle interrupts.
        virtio_device_ready(vdev);

        return 0;
}

static void virtio_simple_remove(struct virtio_device *vdev)
{
        struct virtio_simple_dev *dev = vdev->priv;

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
}

#define VIRTIO_ID_SIMPLE      42
static const struct virtio_device_id id_table[] = {
        {VIRTIO_ID_SIMPLE, VIRTIO_DEV_ANY_ID }, // {DEVICE_ID, VENDOR_ID}
        { 0 },
};

static struct virtio_driver virtio_simple_driver = {
        .driver.name =  KBUILD_MODNAME,
        .id_table =     id_table,
        .probe =        virtio_simple_probe,
        .remove =       virtio_simple_remove,
};

module_virtio_driver(virtio_simple_driver);

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