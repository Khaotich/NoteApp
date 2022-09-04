#include "link_dialog.h"
#include "ui_link_dialog.h"

link_dialog::link_dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::link_dialog)
{
    ui->setupUi(this);
}

link_dialog::~link_dialog()
{
    delete ui;
}

QString link_dialog::getText() const
{
    return ui->text_line_edit->text();
}

QString link_dialog::getTitle() const
{
    return ui->title_line_edit->text();
}

QString link_dialog::getAdress() const
{
    return ui->adress_line_edit->text();
}
