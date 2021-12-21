#ifndef LINK_DIALOG_H
#define LINK_DIALOG_H

#include <QDialog>
#include <QString>

namespace Ui {
class link_dialog;
}

class link_dialog : public QDialog
{
    Q_OBJECT

public:
    explicit link_dialog(QWidget *parent = nullptr);
    ~link_dialog();
    QString getTitle() const;
    QString getText() const;
    QString getAdress() const;

private:
    Ui::link_dialog *ui;
};

#endif // LINK_DIALOG_H
