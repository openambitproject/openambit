#include "logview.h"

LogView::LogView(QWidget *parent):
    QTextBrowser(parent)
{
}

void LogView::showLog(LogEntry *entry)
{
    QString log_html;

    if (entry != NULL && entry->logEntry != NULL && entry->deviceInfo != NULL) {
        log_html += "<h1>" + QString::fromLatin1(entry->logEntry->header.activity_name) + "</h1>";
        if (entry->isUploaded()){
            log_html += "<a href='http://www.movescount.com/moves/move" + entry->movescountId + "'>" + tr("see on movescount.com") + "</a>";
        }
        else {
            log_html += tr("Not uploaded yet");
        }
        log_html += "<h2>" + tr("Details") + "</h2>";
        log_html += "<h4>" + entry->time.toString() + "</h4>";
        log_html += "<h4>" + tr("Duration: %1").arg(QTime(0, 0, 0,0 ).addMSecs(entry->logEntry->header.duration).toString("HH:mm:ss")) + "</h4>";
        log_html += "<h4>" + tr("Distance: %1 m").arg(QString::number(entry->logEntry->header.distance)) + "</h4>";
        log_html += "<h2>" + tr("Training values") + "</h2>";
        log_html += "<h4>" + tr("Avg HR: %1 bpm").arg(QString::number(entry->logEntry->header.heartrate_avg)) + "</h4>";
        log_html += "<h4>" + tr("Max HR: %1 bpm").arg(QString::number(entry->logEntry->header.heartrate_max)) + "</h4>";
        log_html += "<h4>" + tr("Min HR: %1 bpm").arg(QString::number(entry->logEntry->header.heartrate_min)) + "</h4>";
        log_html += "<h4>" + tr("PTE: %1").arg(QString::number(entry->logEntry->header.peak_training_effect/10.0)) + "</h4>";
        log_html += "<h2>" + tr("Device") + "</h2>";
        log_html += "<h4>" + tr("Name: %1").arg(QString(entry->deviceInfo->name)) + "</h4>";
        log_html += "<h4>" + tr("Variant: %1").arg(QString(entry->deviceInfo->model)) + "</h4>";
        log_html += "<h4>" + tr("Serial: %1").arg(QString(entry->deviceInfo->serial)) + "</h4>";

        this->setHtml(log_html);
    }
}

void LogView::hideLog()
{
    this->setHtml("");
}
