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

    void on_button_undo_clicked();

    void on_button_redo_clicked();

    void on_button_emoji_clicked();

    void on_button_text_bold_clicked();

    void on_button_text_italic_clicked();

    void on_button_text_underline_clicked();

    void on_button_text_strike_clicked();

    void on_button_mark_clicked();

    void on_button_horizontal_line_clicked();

    void on_button_h1_clicked();

    void on_button_h2_clicked();

    void on_button_h3_clicked();

    void on_button_quote_clicked();

    void on_button_down_index_clicked();

    void on_button_up_index_clicked();

    void on_button_list_budke_clicked();

    void on_button_list_numeric_clicked();

    void on_button_list_check_clicked();

    void on_button_code_line_clicked();

    void on_button_code_block_clicked();

private:
    Ui::NoteApp *ui;

    Document m_content;
    QString m_filePath;
    bool isModified() const;
};
#endif // NOTEAPP_H
