#ifndef NOTEAPP_H
#define NOTEAPP_H

#include "document.h"

#include <QMainWindow>
#include <QString>
#include <QSystemTrayIcon>
#include <QSqlDatabase>

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
    void on_button_date_timr_clicked();
    void on_button_date_clicked();
    void on_button_time_clicked();
    void on_button_photo_clicked();
    void on_button_link_clicked();

private:
    Ui::NoteApp *ui;

    Document m_content;
    QString m_filePath;
    bool isModified() const;

protected:
    void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;

private slots:
    void showMessage();
    void messageClicked();
    void showHide(QSystemTrayIcon::ActivationReason r);

private:
    void createActions();
    void createTrayIcon();

    QAction *minimizeAction;
    QAction *maximizeAction;
    QAction *restoreAction;
    QAction *quitAction;

    QSystemTrayIcon *trayIcon;
    QMenu *trayIconMenu;

//obsługa bazy danych
private:
    QSqlDatabase database;
    bool openNotebook;
    bool openTag;
    bool openNote;
    QString nameOpenNotebook;
    QString nameOpenTag;
    QString nameOpenNote;
    qint8 idOpenNotebook;
    qint8 idOpenTag;
    qint8 idOpenNote;

private slots:
    void on_button_add_notebook_clicked();
    void on_button_add_tag_clicked();
    void on_button_add_note_clicked();
    void on_editor_textChanged();
    void on_add_tag_to_note_clicked();

protected:
    void open_note(QString name_note);
    void save_note(QString name_note);
    void remove_note(QString name_note);
    void create_note(QString name_note);

    void load_notebooks();
    void load_tags();
    void load_notes_from_nootebook(QString name);
    void load_notes_from_tag(QString name_tag);
    void load_tags_of_note(QString name_note);

    void RemoveLayout (QWidget* widget);
};

#endif // NOTEAPP_H
