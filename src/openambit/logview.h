#ifndef LOGVIEW_H
#define LOGVIEW_H

#include <QTextBrowser>

#include "logentry.h"

class LogView : public QTextBrowser
{
    Q_OBJECT
public:
    explicit LogView(QWidget *parent = 0);

    void showLog(LogEntry *entry);
    void hideLog();

signals:

public slots:

};

#endif // LOGVIEW_H
