#ifndef CONFIRMBETADIALOG_H
#define CONFIRMBETADIALOG_H

#include <QDialog>
#include "settings.h"

namespace Ui {
class ConfirmBetaDialog;
}

class ConfirmBetaDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit ConfirmBetaDialog(QWidget *parent = 0);
    ~ConfirmBetaDialog();

public slots:
     void accept();

private:
    Ui::ConfirmBetaDialog *ui;
    Settings settings;
};

#endif // CONFIRMBETADIALOG_H
