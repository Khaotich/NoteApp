#include "photo_dialog.h"
#include "ui_photo_dialog.h"

#include <QFileDialog>

Photo_Dialog::Photo_Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Photo_Dialog)
{
    ui->setupUi(this);
}

Photo_Dialog::~Photo_Dialog()
{
    delete ui;
}

void Photo_Dialog::on_path_button_clicked()
{
    QString selfilter = tr("All files (*.*)"
                           ";;JPEG (*.jpg *.jpeg)"
                           ";;PNG (*.png)");

    QString path = QFileDialog::getOpenFileName(this,
                                                tr("Insert photo"),
                                                "",
                                                tr("All files (*.*)"
                                                   ";;JPEG (*.jpg *.jpeg)"
                                                   ";;PNG (*.png)"),
                                                &selfilter);
    if(path.isEmpty()) return;
    else
    {
        ui->path_line_edit->setText(path);
    }
}

QString Photo_Dialog::getTitle() const
{
    return ui->title_line_edit->text();
}

QString Photo_Dialog::getAlter() const
{
    return ui->alter_line_edit->text();
}

QString Photo_Dialog::getPath() const
{
    return ui->path_line_edit->text();
}
