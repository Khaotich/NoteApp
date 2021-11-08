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

    void openFile(const QString &path);

private slots:
    void onFileNew();
    void onFileOpen();
    void onFileSave();
    void onFileSaveAs();
    void onExit();

private:
    Ui::NoteApp *ui;

    Document m_content;
    QString m_filePath;
    bool isModified() const;
};
#endif // NOTEAPP_H
