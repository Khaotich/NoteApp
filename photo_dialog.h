#ifndef PHOTO_DIALOG_H
#define PHOTO_DIALOG_H

#include <QDialog>

namespace Ui {
class Photo_Dialog;
}

class Photo_Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Photo_Dialog(QWidget *parent = nullptr);
    ~Photo_Dialog();

    QString getTitle() const;
    QString getAlter() const;
    QString getPath() const;

private slots:
    void on_path_button_clicked();

private:
    Ui::Photo_Dialog *ui;
};

#endif // PHOTO_DIALOG_H
