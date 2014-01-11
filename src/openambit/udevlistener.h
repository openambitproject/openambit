#ifndef UDEVLISTENER_H
#define UDEVLISTENER_H

#include <QObject>
#include <QSocketNotifier>

#include <libudev.h>

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
    struct udev *udev;
    struct udev_monitor *mon;
    int fd;
    QSocketNotifier *socketNotifier;
};

#endif // UDEVLISTENER_H
