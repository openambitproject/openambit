#include "udevlistener.h"

#define SUUNTO_USB_VENDOR_ID "1493"

UdevListener::UdevListener(QObject *parent) :
    QObject(parent)
{
#if UDEV_FOUND
    udev = udev_new();

    if (!udev) {
        return;
    }

    /* Set up a monitor to monitor hid devices */
    mon = udev_monitor_new_from_netlink(udev, "udev");
    udev_monitor_filter_add_match_subsystem_devtype(mon, "hid", NULL);
    udev_monitor_enable_receiving(mon);

    fd = udev_monitor_get_fd(mon);

    socketNotifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
    connect(socketNotifier, SIGNAL(activated(int)), this, SLOT(fdActivated(int)));
#endif
}

UdevListener::~UdevListener()
{
#if UDEV_FOUND
    delete socketNotifier;

    udev_monitor_unref(mon);
    udev_unref(udev);
#endif
}

void UdevListener::fdActivated(int fd)
{
#if UDEV_FOUND
    struct udev_device *device;
    const char *vendorId;

    if (fd == this->fd) {
        device = udev_monitor_receive_device(mon);

        if (device != NULL) {
            // Check vendor ID, if it matches Suunto, we issue a new device scan
            vendorId = udev_device_get_sysattr_value(device, "idVendor");
            if (vendorId != NULL && strcmp(vendorId, SUUNTO_USB_VENDOR_ID) == 0) {
                emit deviceEvent();
            }

            udev_device_unref(device);
        }
    }
#endif
}
