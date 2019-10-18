#ifndef UDEVLISTENER_H
#define UDEVLISTENER_H

#include <QObject>
#include <QSocketNotifier>
#if UDEV_FOUND
#include <libudev.h>
#endif

class UdevListener : public QObject
{
    Q_OBJECT
public:
    explicit UdevListener(QObject *parent = 0);
    ~UdevListener();

signals:
    void deviceEvent();

private slots:
    void fdActivated(int fd);

private:
#if UDEV_FOUND
    struct udev *udev;
    struct udev_monitor *mon;
#endif
    int fd;
    QSocketNotifier *socketNotifier;
};

#endif // UDEVLISTENER_H
