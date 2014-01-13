#include "confirmbetadialog.h"
#include "ui_confirmbetadialog.h"

ConfirmBetaDialog::ConfirmBetaDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConfirmBetaDialog)
{
    ui->setupUi(this);
}

ConfirmBetaDialog::~ConfirmBetaDialog()
{
    delete ui;
}

void ConfirmBetaDialog::accept()
{
    settings.beginGroup("generalSettings");
    settings.setValue("skipBetaCheck", ui->skipBetaCheck->isChecked());
    settings.endGroup();
    QDialog::accept();
}
