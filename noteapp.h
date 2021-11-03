#ifndef NOTEAPP_H
#define NOTEAPP_H

#include "document.h"

#include <QMainWindow>
#include <QString>

QT_BEGIN_NAMESPACE
namespace Ui {class NoteApp;}
QT_END_NAMESPACE

class NoteApp : public QMainWindow
{
    Q_OBJECT

public:
    NoteApp(QWidget *parent = nullptr);
    ~NoteApp();

private:
    Ui::NoteApp *ui;

    Document m_content;
};
#endif // NOTEAPP_H
