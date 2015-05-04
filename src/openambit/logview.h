#ifndef LOGVIEW_H
#define LOGVIEW_H

#include <QTextBrowser>

#include <movescount/logentry.h>

class LogView : public QTextBrowser
{
    Q_OBJECT
public:
    explicit LogView(QWidget *parent = 0);

    void showLog(LogEntry *entry);
    void hideLog();

signals:

public slots:

private:
    QString msecToHHMMSS(quint32 msec);

};

#endif // LOGVIEW_H
