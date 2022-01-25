#include "noteapp.h"
#include "ui_noteapp.h"
#include "previewpage.h"
#include "link_dialog.h"
#include "photo_dialog.h"

#include <QFile>
#include <QDialog>
#include <QFileDialog>
#include <QFontDatabase>
#include <QMessageBox>
#include <QTextStream>
#include <QWebChannel>
#include <QKeyEvent>
#include <QDateTime>
#include <QAction>
#include <QCoreApplication>
#include <QCloseEvent>
#include <QMenu>
#include <QtSql>
#include <QInputDialog>

#include <Windows.h>


//-------------------------------------------------------------------------------------------//
//----------------------------Inicjalizacja aplikacji QT-------------------------------------//


//konstruktor aplikacji
NoteApp::NoteApp(QWidget *parent): QMainWindow(parent), ui(new Ui::NoteApp)
{
    ui->setupUi(this);
    ui->editor->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    ui->preview->setContextMenuPolicy(Qt::NoContextMenu);

    //iniciuje i zmieniam wygląd widoku preview
    PreviewPage *page = new PreviewPage(this);
    ui->preview->setPage(page);
    ui->preview->resize(260, 1);

    //ustawiam czcionkę dla edytora
    QFont font = ui->editor->font();
    font.setPointSize(12);
    ui->editor->setFont(font);

    //podłączam edytor do widoku preview
    QWebChannel *channel = new QWebChannel(this);
    channel->registerObject(QStringLiteral("content"), &m_content);
    page->setWebChannel(channel);
    ui->preview->setUrl(QUrl("qrc:/index.html"));

    //podłączam akcje do slotów dla plików markdown
    connect(ui->actionSaveAs, &QAction::triggered, this, &NoteApp::onFileSaveAs);
    connect(ui->actionExit, &QAction::triggered, this, &NoteApp::onExit);
    connect(ui->editor, &QPlainTextEdit::textChanged, [this]() { m_content.setText(ui->editor->toPlainText()); });

    //wczytanie ciemnego motywu aplikacji
    QFile f(":qdarkstyle/dark/style.qss");
    if(!f.exists())
    {
        printf("Unable to set stylesheet, file not found\n");
    }
    else
    {
        f.open(QFile::ReadOnly | QFile::Text);
        QTextStream ts(&f);
        qApp->setStyleSheet(ts.readAll());
    }

    //ustawienie ikony zasobnika aplikacji
    createActions();
    createTrayIcon();
    connect(trayIcon, &QSystemTrayIcon::messageClicked, this, &NoteApp::messageClicked);
    connect(trayIcon,SIGNAL(activated(QSystemTrayIcon::ActivationReason)),this,SLOT(showHide(QSystemTrayIcon::ActivationReason)));


    //inicjalizacja bazy danych i systemu notatek
    database = QSqlDatabase::addDatabase("QSQLITE");
    database.setDatabaseName("database.db");
    database.open();

    load_notebooks();
    load_tags();

    openNotebook = false;
    openNote = false;
    openTag = false;
    nameOpenNotebook = "";
    nameOpenNote = "";
    nameOpenTag = "";
    idOpenNotebook = 0;
    idOpenTag = 0;
    idOpenNote = 0;
}

//destruktor aplikacji
NoteApp::~NoteApp()
{
    database.close();
    delete ui;
}


//-------------------------------------------------------------------------------------------//
//---------------------------------Obsługa plików--------------------------------------------//


//zapisanie pliku
void NoteApp::onFileSave()
{
    if(m_filePath.isEmpty())
    {
        onFileSaveAs();
        return;
    }

    QFile f(m_filePath);
    if(!f.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::warning(this, windowTitle(), tr("Could not write to file %1: %2").arg(QDir::toNativeSeparators(m_filePath), f.errorString()));
        return;
    }

    QTextStream str(&f);
    str << ui->editor->toPlainText();
    ui->editor->document()->setModified(false);
}

//zapisanie pliku 'jako'
void NoteApp::onFileSaveAs()
{
    QString path = QFileDialog::getSaveFileName(this, tr("Save MarkDown File"), "", tr("MarkDown File (*.md *.markdown)"));
    if(path.isEmpty()) return;
    m_filePath = path;
    onFileSave();
}

//wyjście
void NoteApp::onExit()
{
    close();
}


//-------------------------------------------------------------------------------------------//
//---------------------------------Obsługa edytora-------------------------------------------//


//przycisk wstecz
void NoteApp::on_button_undo_clicked()
{
    ui->editor->undo();
    ui->editor->setFocus();
}

//przycisk w przód
void NoteApp::on_button_redo_clicked()
{
    ui->editor->redo();
    ui->editor->setFocus();
}

//przycisk emoji (wykorzystuje windows_api)
void NoteApp::on_button_emoji_clicked()
{
    keybd_event(VK_LWIN, 0, 0, 0);
    keybd_event(VK_OEM_PERIOD, 0, 0, 0);
    keybd_event(VK_OEM_PERIOD, 0, KEYEVENTF_KEYUP, 0);
    keybd_event(VK_LWIN, 0, KEYEVENTF_KEYUP, 0);

    ui->editor->setFocus();
}

//przycisk pogrubiony tekst
void NoteApp::on_button_text_bold_clicked()
{
    QString text = ui->editor->toPlainText();
    text += "****";
    ui->editor->setPlainText(text);

    QTextCursor tc = ui->editor->textCursor();
    tc.setPosition(ui->editor->document()->characterCount() - 3);
    ui->editor->setTextCursor(tc);

    ui->editor->setFocus();
}

//przycisk pochylony teskt
void NoteApp::on_button_text_italic_clicked()
{
    QString text = ui->editor->toPlainText();
    text += "**";
    ui->editor->setPlainText(text);

    QTextCursor tc = ui->editor->textCursor();
    tc.setPosition(ui->editor->document()->characterCount() - 2);
    ui->editor->setTextCursor(tc);

    ui->editor->setFocus();
}

//przycisk tekst podkreślony
void NoteApp::on_button_text_underline_clicked()
{
    QString text = ui->editor->toPlainText();
    text += "<ins></ins>";
    ui->editor->setPlainText(text);

    QTextCursor tc = ui->editor->textCursor();
    tc.setPosition(ui->editor->document()->characterCount() - 7);
    ui->editor->setTextCursor(tc);

    ui->editor->setFocus();
}

//przycisk tekst przekreślony
void NoteApp::on_button_text_strike_clicked()
{
    QString text = ui->editor->toPlainText();
    text += "~~~~";
    ui->editor->setPlainText(text);

    QTextCursor tc = ui->editor->textCursor();
    tc.setPosition(ui->editor->document()->characterCount() - 3);
    ui->editor->setTextCursor(tc);

    ui->editor->setFocus();
}

//przycisk tekst z markerem
void NoteApp::on_button_mark_clicked()
{
    QString text = ui->editor->toPlainText();
    text += "<mark></mark>";
    ui->editor->setPlainText(text);

    QTextCursor tc = ui->editor->textCursor();
    tc.setPosition(ui->editor->document()->characterCount() - 8);
    ui->editor->setTextCursor(tc);

    ui->editor->setFocus();
}

//przycisk linia pozioma
void NoteApp::on_button_horizontal_line_clicked()
{
    QString text = ui->editor->toPlainText();
    text += "***\n";
    ui->editor->setPlainText(text);

    QTextCursor tc = ui->editor->textCursor();
    tc.setPosition(ui->editor->document()->characterCount() - 1);
    ui->editor->setTextCursor(tc);

    ui->editor->setFocus();
}

//przycisk cytat
void NoteApp::on_button_quote_clicked()
{
    QString text = ui->editor->toPlainText();
    text += "> ";
    ui->editor->setPlainText(text);

    QTextCursor tc = ui->editor->textCursor();
    tc.setPosition(ui->editor->document()->characterCount() - 1);
    ui->editor->setTextCursor(tc);

    ui->editor->setFocus();
}

//przycisk nagłówek 1
void NoteApp::on_button_h1_clicked()
{
    QString text = ui->editor->toPlainText();
    text += "# ";
    ui->editor->setPlainText(text);

    QTextCursor tc = ui->editor->textCursor();
    tc.setPosition(ui->editor->document()->characterCount() - 1);
    ui->editor->setTextCursor(tc);

    ui->editor->setFocus();
}

//przycisk nagłówek 2
void NoteApp::on_button_h2_clicked()
{
    QString text = ui->editor->toPlainText();
    text += "## ";
    ui->editor->setPlainText(text);

    QTextCursor tc = ui->editor->textCursor();
    tc.setPosition(ui->editor->document()->characterCount() - 1);
    ui->editor->setTextCursor(tc);

    ui->editor->setFocus();
}

//przycisk nagłówek 3
void NoteApp::on_button_h3_clicked()
{
    QString text = ui->editor->toPlainText();
    text += "### ";
    ui->editor->setPlainText(text);

    QTextCursor tc = ui->editor->textCursor();
    tc.setPosition(ui->editor->document()->characterCount() - 1);
    ui->editor->setTextCursor(tc);

    ui->editor->setFocus();
}

//przycisk indeks dolny
void NoteApp::on_button_down_index_clicked()
{
    QString text = ui->editor->toPlainText();
    text += "<sub></sub>";
    ui->editor->setPlainText(text);

    QTextCursor tc = ui->editor->textCursor();
    tc.setPosition(ui->editor->document()->characterCount() - 7);
    ui->editor->setTextCursor(tc);

    ui->editor->setFocus();
}

//przycisk indeks górny
void NoteApp::on_button_up_index_clicked()
{
    QString text = ui->editor->toPlainText();
    text += "<sup></sup>";
    ui->editor->setPlainText(text);

    QTextCursor tc = ui->editor->textCursor();
    tc.setPosition(ui->editor->document()->characterCount() - 7);
    ui->editor->setTextCursor(tc);

    ui->editor->setFocus();
}

//przycisk lista punktowa
void NoteApp::on_button_list_budke_clicked()
{
    QTextCursor cursor = ui->editor->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.select(QTextCursor::LineUnderCursor);
    QString line = cursor.selectedText();

    if(line[0]=='-' && line[1]==' ')
    {
        QString text = ui->editor->toPlainText();
        text += "\n- ";
        ui->editor->setPlainText(text);

        QTextCursor tc = ui->editor->textCursor();
        tc.setPosition(ui->editor->document()->characterCount() - 1);
        ui->editor->setTextCursor(tc);

        ui->editor->setFocus();
    }
    else
    {
        QString text = ui->editor->toPlainText();
        text += "- ";
        ui->editor->setPlainText(text);

        QTextCursor tc = ui->editor->textCursor();
        tc.setPosition(ui->editor->document()->characterCount() - 1);
        ui->editor->setTextCursor(tc);

        ui->editor->setFocus();
    }
}

//przycisk lista wypunktowana
void NoteApp::on_button_list_numeric_clicked()
{
    QTextCursor cursor = ui->editor->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.select(QTextCursor::LineUnderCursor);
    QString line = cursor.selectedText();

    if(line[0].isDigit())
    {
        qint32 num = line.split(".")[0].toInt();
        QString text = ui->editor->toPlainText();
        text += "\n" + QString::number(num + 1) + ". ";
        ui->editor->setPlainText(text);

        QTextCursor tc = ui->editor->textCursor();
        tc.setPosition(ui->editor->document()->characterCount() - 1);
        ui->editor->setTextCursor(tc);

        ui->editor->setFocus();
    }
    else
    {
        QString text = ui->editor->toPlainText();
        text += "\n1. ";
        ui->editor->setPlainText(text);

        QTextCursor tc = ui->editor->textCursor();
        tc.setPosition(ui->editor->document()->characterCount() - 1);
        ui->editor->setTextCursor(tc);

        ui->editor->setFocus();
    }
}

//przycisk lista checkboxów
void NoteApp::on_button_list_check_clicked()
{
    QString text = ui->editor->toPlainText();
    text += "\n - [ ] ";
    ui->editor->setPlainText(text);

    QTextCursor tc = ui->editor->textCursor();
    tc.setPosition(ui->editor->document()->characterCount() - 1);
    ui->editor->setTextCursor(tc);

    ui->editor->setFocus();
}

//przycisk linia kodu
void NoteApp::on_button_code_line_clicked()
{
    QString text = ui->editor->toPlainText();
    text += "``";
    ui->editor->setPlainText(text);

    QTextCursor tc = ui->editor->textCursor();
    tc.setPosition(ui->editor->document()->characterCount() - 2);
    ui->editor->setTextCursor(tc);

    ui->editor->setFocus();
}

//przycisk blok kodu
void NoteApp::on_button_code_block_clicked()
{
    QString text = ui->editor->toPlainText();
    text += "\n```\n\n```";
    ui->editor->setPlainText(text);

    QTextCursor tc = ui->editor->textCursor();
    tc.setPosition(ui->editor->document()->characterCount() - 5);
    ui->editor->setTextCursor(tc);

    ui->editor->setFocus();
}

//przycisk z pełną datą
void NoteApp::on_button_date_timr_clicked()
{
    QDateTime now = QDateTime::currentDateTime();
    QString now_ = now.toString("dd.MM.yyyy hh:mm");

    QString text = ui->editor->toPlainText();
    text += " " + now_ + " ";
    ui->editor->setPlainText(text);

    QTextCursor tc = ui->editor->textCursor();
    tc.setPosition(ui->editor->document()->characterCount() - 1);
    ui->editor->setTextCursor(tc);

    ui->editor->setFocus();
}

//przycisk z samą datą
void NoteApp::on_button_date_clicked()
{
    QDateTime now = QDateTime::currentDateTime();
    QString now_ = now.toString("dd.MM.yyyy");

    QString text = ui->editor->toPlainText();
    text += " " + now_ + " ";
    ui->editor->setPlainText(text);

    QTextCursor tc = ui->editor->textCursor();
    tc.setPosition(ui->editor->document()->characterCount() - 1);
    ui->editor->setTextCursor(tc);

    ui->editor->setFocus();
}

//przycisk z samą godziną
void NoteApp::on_button_time_clicked()
{
    QDateTime now = QDateTime::currentDateTime();
    QString now_ = now.toString("hh:mm:ss");

    QString text = ui->editor->toPlainText();
    text += " " + now_ + " ";
    ui->editor->setPlainText(text);

    QTextCursor tc = ui->editor->textCursor();
    tc.setPosition(ui->editor->document()->characterCount() - 1);
    ui->editor->setTextCursor(tc);

    ui->editor->setFocus();
}

//przycisk dodawanie zdjęcia
void NoteApp::on_button_photo_clicked()
{
    Photo_Dialog dialog;
    dialog.setModal(true);
    dialog.setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowTitleHint);

    if(dialog.exec() == QDialog::Accepted)
    {
        QString path = dialog.getPath();
        QString title = dialog.getTitle();
        QString alter = dialog.getAlter();

        QFileInfo fi(path);
        QString fileName = fi.fileName();
        QString prefix = "file:///photos/";
        QString destinationPathMarkDown =  prefix + fileName;
        QString destinationPath = "photos/" + fileName;

        if(QFile::exists(destinationPath)) QFile::remove(destinationPath);
        if(QFile::copy(path, destinationPath))
        {
            QString text = ui->editor->toPlainText();
            text += "![" + alter + "](" + destinationPathMarkDown + " '" + title + "')";
            ui->editor->setPlainText(text);

            QTextCursor tc = ui->editor->textCursor();
            tc.setPosition(ui->editor->document()->characterCount() - 1);
            ui->editor->setTextCursor(tc);

            ui->editor->setFocus();
        }
        else return;
    }
}

//przycisk link
void NoteApp::on_button_link_clicked()
{
    QString text_ = "";
    QString title = "";
    QString adress = "";

    link_dialog dialog;
    dialog.setModal(true);
    dialog.setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowTitleHint);

    if(dialog.exec() == QDialog::Accepted)
    {
        text_ = dialog.getText();
        title = dialog.getTitle();
        adress = dialog.getAdress();

        if(adress.right(1) == "/") adress.remove(adress.length() - 1, 1);
        if(adress.left(8) == "https://") adress.remove(0, 8);
        if(adress.left(7) == "http://") adress.remove(0, 7);

        QString content = "["+ text_ + "]"
                + "(http://" + adress
                + " \"" + title + "\")";

        QString text = ui->editor->toPlainText();
        text += content;
        ui->editor->setPlainText(text);

        QTextCursor tc = ui->editor->textCursor();
        tc.setPosition(ui->editor->document()->characterCount() - 1);
        ui->editor->setTextCursor(tc);

        ui->editor->setFocus();
    }
}


//-------------------------------------------------------------------------------------------//
//---------------------------------Obsługa zasobnika-----------------------------------------//


//obsługa kliknięć ikony w zasobniku
void NoteApp::showHide(QSystemTrayIcon::ActivationReason r)
{
    if(r == QSystemTrayIcon::Trigger)
    {
        if (!this->isVisible()) this->show();
        else this->hide();
    }
}

//nadpisujemy zdarzenie zamkniecia aplikacji
void NoteApp::closeEvent(QCloseEvent *event)
{
    if(trayIcon->isVisible())
    {
        hide();
        showMessage();
        event->ignore();
    }
}

//pokazanie powiadomienia
void NoteApp::showMessage()
{
    QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::MessageIcon(0);
    trayIcon->showMessage("NoteApp work in background",
                          "NoteApp will be working in tray system, click icon or notification to show app",
                          icon);
}

//obsługa kliknięcia powiadomienia
void NoteApp::messageClicked()
{
    show();
}

//przypisujemy akcje do menu ikony zasobnika
void NoteApp::createActions()
{
    minimizeAction = new QAction(tr("Mi&nimize"), this);
    connect(minimizeAction, &QAction::triggered, this, &QWidget::hide);

    maximizeAction = new QAction(tr("Ma&ximize"), this);
    connect(maximizeAction, &QAction::triggered, this, &QWidget::showMaximized);

    restoreAction = new QAction(tr("&Restore"), this);
    connect(restoreAction, &QAction::triggered, this, &QWidget::showNormal);

    quitAction = new QAction(tr("&Quit"), this);
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);
}

//tworzymy menu i ikonę zasobnika
void NoteApp::createTrayIcon()
{
    trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(minimizeAction);
    trayIconMenu->addAction(maximizeAction);
    trayIconMenu->addAction(restoreAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);

    QIcon icon = QIcon(":/NoteApp.png");
    trayIcon = new QSystemTrayIcon(this);

    trayIcon->setIcon(icon);
    trayIcon->setToolTip("NoteApp");
    trayIcon->setContextMenu(trayIconMenu);
    trayIcon->show();
}


//-------------------------------------------------------------------------------------------//
//------------------------------Funkcje systemu notatek--------------------------------------//


//otwarcie notatki
void NoteApp::open_note(QString name_note)
{
    if(database.isOpen())
    {
        QSqlQuery query("SELECT Id, Date FROM Notes WHERE NoteName='" + name_note + "'");
        query.exec();
        query.next();
        idOpenNote = query.value(0).toInt();
        QString date = query.value(1).toString();

        nameOpenNote = name_note;
        openNote = true;

        QFile f("notes/" + name_note + ".md");
        f.open(QIODevice::ReadOnly);
        ui->editor->setPlainText(f.readAll());
        f.close();

        load_tags_of_note(name_note);

        QTextCursor tc = ui->editor->textCursor();
        tc.setPosition(ui->editor->document()->characterCount() - 1);
        ui->editor->setTextCursor(tc);
        ui->editor->setFocus();

        ui->title->setText(name_note);
        ui->date->setText(date);
    }
    else return;
}

//zapisanie notatki
void NoteApp::save_note(QString name_note)
{
    QFile f("notes/" + name_note + ".md");
    f.open(QIODevice::WriteOnly | QIODevice::Truncate);
    QString tmp = ui->editor->toPlainText();
    f.write(tmp.toUtf8());
    f.close();
}

//tworzenie notatki
void NoteApp::create_note(QString name_note)
{
    QFile f("notes/" + name_note + ".md");
    f.open(QIODevice::WriteOnly);
    f.close();
    open_note(name_note);
}

//usunięcie notatki
void NoteApp::remove_note(QString name_note)
{
    if(name_note == nameOpenNote)
    {
        nameOpenNote.clear();
        openNote = false;
        ui->title->clear();
        ui->date->clear();
        ui->editor->clear();

        if(ui->tags_from_note->layout() != NULL)
        {
            QLayoutItem* item;
            while((item = ui->tags_from_note->layout()->takeAt(0)) != NULL)
            {
                QWidget* widget = item->widget();
                delete widget;
                delete item;
            }
            delete ui->tags_from_note->layout();
        }
    }

    if(database.isOpen())
    {
        QSqlQuery queryIdNote("SELECT Id FROM Notes WHERE NoteName='" + name_note + "'");
        queryIdNote.exec();
        queryIdNote.next();
        QString idNote = queryIdNote.value(0).toString();

        QSqlQuery queryRemove1("DELETE FROM TagsNotes WHERE IdNote='" + idNote + "'");
        QSqlQuery queryRemove2("DELETE FROM Notes WHERE Id='" + idNote + "'");
        QSqlQuery queryRemove3("DELETE FROM NotebooksNotes WHERE IdNote='" + idNote + "'");

        queryRemove1.exec();
        queryRemove2.exec();
        queryRemove3.exec();
    }

    if(QFile::exists("notes/" + name_note + ".md"))
    {
        QFile::remove("notes/" + name_note + ".md");
    }

    if(openTag) load_notes_from_tag(nameOpenTag);
    else if(openNotebook) load_notes_from_nootebook(nameOpenNotebook);
}

//zapisuje plik przy każdej zmianie tekstu
void NoteApp::on_editor_cursorPositionChanged()
{
    if(openNote && ui->editor->hasFocus()) save_note(nameOpenNote);
}

//usuwanie tagów z notatki
void NoteApp::remove_tag_from_note(QString name_tag)
{
    if(database.isOpen())
    {
        QSqlQuery queryIdTag("SELECT Id FROM Tags WHERE TagName='" + name_tag + "'");
        queryIdTag.exec();
        queryIdTag.next();
        QString idTag = queryIdTag.value(0).toString();

        QSqlQuery queryRemove("DELETE FROM TagsNotes WHERE IdNote='" + QString::number(idOpenNote) + "' AND IdTag='" + idTag + "'");
        if(queryRemove.exec()) load_tags_of_note(nameOpenNote);
    }
}

//usuwanie tagów z systemu
void NoteApp::remove_tag(QString name_tag)
{
    QSqlQuery queryIdTag("SELECT Id FROM Tags WHERE TagName='" + name_tag + "'");
    queryIdTag.exec();
    queryIdTag.next();
    QString idTag = queryIdTag.value(0).toString();

    QSqlQuery queryRemove("DELETE FROM TagsNotes WHERE IdTag='" + idTag + "'");
    QSqlQuery queryRemove_("DELETE FROM Tags WHERE Id='" + idTag + "'");

    if(queryRemove.exec() && queryRemove_.exec())
    {
        load_tags();
        load_tags_of_note(nameOpenNote);
    }
}

//usuwanie notatników z systemu
void NoteApp::remove_notepad(QString name_notepad)
{
    QMessageBox mes;
    auto result = QMessageBox::warning(this, "Remove notepad", "Are you sure to delete the\nnotepad with all its notes?", QMessageBox::Yes|QMessageBox::No);

    if(result == QMessageBox::Yes)
    {
        if(database.isOpen())
        {
            if(name_notepad == nameOpenNotebook)
            {
                openNotebook = false;
                nameOpenNotebook = "";
                idOpenNotebook = 0;

                QVBoxLayout *layout = new QVBoxLayout;
                QWidget *widget = new QWidget;
                layout->setAlignment(Qt::AlignTop);
                widget->setLayout(layout);
                ui->scroll_area_notes->setWidget(widget);
            }

            QSqlQuery query("SELECT Notes.NoteName FROM Notes, NotebooksNotes, Notebooks WHERE NotebooksNotes.IdNotebook=Notebooks.Id AND NotebooksNotes.IdNote=Notes.Id AND Notebooks.NotebookName='" + name_notepad + "'");
            int idName = query.record().indexOf("NoteName");
            while(query.next())
            {
                QString name_ = query.value(idName).toString();
                remove_note(name_);
            }

            QSqlQuery queryRemove("DELETE FROM Notebooks WHERE NotebookName='" + name_notepad + "'");
            queryRemove.exec();
            load_notebooks();
        }
    }
    else return;
}


//-------------------------------------------------------------------------------------------//
//--------------------------------Obsługa bazy danych----------------------------------------//


//przycisk dodanie notatnika
void NoteApp::on_button_add_notebook_clicked()
{
    bool ok = false;
    QString name = QInputDialog::getText(0, "Add notebook",
                                         "Notenook Name:",
                                         QLineEdit::Normal,
                                         "", &ok,
                                         Qt::CustomizeWindowHint
                                         | Qt::WindowTitleHint);

    if(ok && !name.isEmpty() && database.isOpen())
    {
        QSqlQuery query;
        query.prepare("INSERT INTO Notebooks (NotebookName) VALUES (:name)");
        query.bindValue(":name", name);
        if(query.exec()) load_notebooks();
    }
    else return;
}

//wczytanie notatników
void NoteApp::load_notebooks()
{
    if(database.isOpen())
    {
        QSqlQuery query("SELECT NotebookName FROM Notebooks");
        int idName = query.record().indexOf("NotebookName");

        QVBoxLayout *layout = new QVBoxLayout;
        QWidget *widget = new QWidget;
        layout->setAlignment(Qt::AlignTop);
        widget->setLayout(layout);
        ui->scroll_area_notebooks->setWidget(widget);

        while(query.next())
        {
            QString name = query.value(idName).toString();

            QPushButton *button = new QPushButton();
            button->setText(name);
            layout->addWidget(button);
            connect(button, &QPushButton::clicked, button, [=]{load_notes_from_nootebook(name);});

            button->setContextMenuPolicy(Qt::CustomContextMenu);
            connect(button, &QPushButton::customContextMenuRequested, button, [=]{
                QMenu* menu = new QMenu();
                QAction* action = new QAction("Remove");
                menu->addAction(action);
                QPoint global_pos = button->mapToGlobal(QPoint(0,0));
                connect(action, &QAction::triggered, action, [=]{
                    remove_notepad(name);
                 });
                menu->exec(global_pos+button->rect().center());
            });
        }
    }
    else return;
}

//ładowanie notatek z notesu
void NoteApp::load_notes_from_nootebook(QString name)
{
    if(openNote)
    {
        save_note(nameOpenNote);
        ui->editor->setPlainText("");
        openNote = false;
        nameOpenNote = "";
        idOpenNote = 0;
    }

    openNotebook = true;
    openTag = false;
    nameOpenNotebook = name;
    nameOpenTag = "";
    idOpenTag = 0;

    ui->title->clear();
    ui->date->clear();

    QWidget * tmp = ui->tags_from_note;
    if(tmp->layout() != 0)
    {
        QLayoutItem* item;
        while((item = tmp->layout()->takeAt(0)) != 0)
        {
            delete item->widget();
            delete item;
        }
        delete tmp->layout();
    }

    if(database.isOpen())
    {
        QSqlQuery query_id("SELECT Id FROM Notebooks WHERE NotebookName='" + name + "'");
        query_id.exec();
        query_id.next();
        idOpenNotebook = query_id.value(0).toInt();

        QSqlQuery query("SELECT Notes.NoteName FROM Notes, NotebooksNotes, Notebooks WHERE NotebooksNotes.IdNotebook=Notebooks.Id AND NotebooksNotes.IdNote=Notes.Id AND Notebooks.NotebookName='" + name+ "'");
        int idName = query.record().indexOf("NoteName");

        QVBoxLayout *layout = new QVBoxLayout;
        QWidget *widget = new QWidget;
        layout->setAlignment(Qt::AlignTop);
        widget->setLayout(layout);
        ui->scroll_area_notes->setWidget(widget);

        while(query.next())
        {
            QString name_ = query.value(idName).toString();

            QPushButton *button = new QPushButton();
            button->setText(name_);
            layout->addWidget(button);
            button->setContextMenuPolicy(Qt::CustomContextMenu);

            connect(button, &QPushButton::clicked, button, [=]{open_note(name_);});
            connect(button, &QPushButton::customContextMenuRequested, button, [=]{
                QMenu* menu = new QMenu();
                QAction* action = new QAction("Remove");
                menu->addAction(action);
                QPoint global_pos = button->mapToGlobal(QPoint(0,0));
                connect(action, &QAction::triggered, action, [=]{
                    remove_note(name_);
                 });
                menu->exec(global_pos+button->rect().center());
            });
        }
    }
    else return;
}

//ładowanie tagów
void NoteApp::load_tags()
{
    if(database.isOpen())
    {
        QSqlQuery query("SELECT TagName FROM Tags");
        int idName = query.record().indexOf("TagName");

        QVBoxLayout *layout = new QVBoxLayout;
        QWidget *widget = new QWidget;
        layout->setAlignment(Qt::AlignTop);
        widget->setLayout(layout);
        ui->scroll_area_tags->setWidget(widget);

        while(query.next())
        {
            QString name = query.value(idName).toString();

            QPushButton *button = new QPushButton();
            button->setText(name);
            layout->addWidget(button);
            connect(button, &QPushButton::clicked, button, [=]{load_notes_from_tag(name);});

            button->setContextMenuPolicy(Qt::CustomContextMenu);
            connect(button, &QPushButton::customContextMenuRequested, button, [=]{
                QMenu* menu = new QMenu();
                QAction* action = new QAction("Remove");
                menu->addAction(action);
                QPoint global_pos = button->mapToGlobal(QPoint(0,0));
                connect(action, &QAction::triggered, action, [=]{
                    remove_tag(name);
                 });
                menu->exec(global_pos+button->rect().center());
            });
        }
    }
    else return;
}

//ładowanie notatek z tagów
void NoteApp::load_notes_from_tag(QString name_tag)
{
    if(openNote)
    {
        save_note(nameOpenNote);
        ui->editor->setPlainText("");
        openNote = false;
        nameOpenNote = "";
        idOpenNote = 0;
    }

    openNotebook = false;
    openTag = true;
    nameOpenNotebook = "";
    nameOpenTag = name_tag;
    idOpenNotebook = 0;

    ui->title->clear();
    ui->date->clear();

    QWidget * tmp = ui->tags_from_note;
    if(tmp->layout() != 0)
    {
        QLayoutItem* item;
        while((item = tmp->layout()->takeAt(0)) != 0)
        {
            delete item->widget();
            delete item;
        }
        delete tmp->layout();
    }

    if(database.isOpen())
    {
        QSqlQuery query_id("SELECT Id FROM Tags WHERE TagName='" + name_tag + "'");
        query_id.exec();
        query_id.next();
        idOpenTag = query_id.value(0).toInt();

        QSqlQuery query("SELECT Notes.NoteName FROM Notes, Tags, TagsNotes WHERE TagsNotes.IdNote=Notes.Id AND TagsNotes.IdTag=Tags.Id AND Tags.TagName='" + name_tag + "'");
        int idName = query.record().indexOf("NoteName");

        QVBoxLayout *layout = new QVBoxLayout;
        QWidget *widget = new QWidget;
        layout->setAlignment(Qt::AlignTop);
        widget->setLayout(layout);
        ui->scroll_area_notes->setWidget(widget);

        while(query.next())
        {
            QString name_ = query.value(idName).toString();

            QPushButton *button = new QPushButton();
            button->setText(name_);
            layout->addWidget(button);
            connect(button, &QPushButton::clicked, button,[=]{open_note(name_);});

            button->setContextMenuPolicy(Qt::CustomContextMenu);
            connect(button, &QPushButton::customContextMenuRequested, button, [=]{
                QMenu* menu = new QMenu();
                QAction* action = new QAction("Remove");
                menu->addAction(action);
                QPoint global_pos = button->mapToGlobal(QPoint(0,0));
                connect(action, &QAction::triggered, action, [=]{
                    remove_note(name_);
                 });
                menu->exec(global_pos+button->rect().center());
            });
        }
    }
    else return;
}

//przycisk dodawania tagów
void NoteApp::on_button_add_tag_clicked()
{
    bool ok = false;
    QString name = QInputDialog::getText(0, "Add tag",
                                         "Tag Name:",
                                         QLineEdit::Normal,
                                         "", &ok,
                                         Qt::CustomizeWindowHint
                                         | Qt::WindowTitleHint);

    if(ok && !name.isEmpty() && database.isOpen())
    {
        QSqlQuery query;
        query.prepare("INSERT INTO Tags (TagName) VALUES (:name)");
        query.bindValue(":name", name);
        if(query.exec()) load_tags();
    }
    else return;
}

//przycisk dodania notatki
void NoteApp::on_button_add_note_clicked()
{
    if(openNotebook)
    {
        bool ok = false;
        QString name = QInputDialog::getText(0, "Add note",
                                             "Note Name:",
                                             QLineEdit::Normal,
                                             "", &ok,
                                             Qt::CustomizeWindowHint
                                             | Qt::WindowTitleHint);

        if(ok && !name.isEmpty() && database.isOpen())
        {
            QDateTime time_ = QDateTime::currentDateTime();
            QString time = time_.toString("dd.MM.yyyy hh:mm");

            QSqlQuery query_insert_note, query_insert_notebooksnotes;

            query_insert_note.prepare("INSERT INTO Notes (NoteName, Date) VALUES (:name, :time)");
            query_insert_note.bindValue(":name", name);
            query_insert_note.bindValue(":time", time);

            query_insert_note.exec();

            QSqlQuery query_get_id_note("SELECT Id FROM Notes");
            query_get_id_note.exec();
            query_get_id_note.last();
            qint32 idNote = query_get_id_note.value(0).toInt();

            query_insert_notebooksnotes.prepare("INSERT INTO NotebooksNotes (IdNotebook, IdNote) VALUES (:idNotebook, :idNote)");
            query_insert_notebooksnotes.bindValue(":idNotebook", idOpenNotebook);
            query_insert_notebooksnotes.bindValue(":idNote", idNote);

            if(query_insert_notebooksnotes.exec()) create_note(name);
            load_notes_from_nootebook(nameOpenNotebook);
        }
        else return;
    }
    else
    {
        QMessageBox messageBox;
        messageBox.warning(0,"Warning","Add note only in notebook mode!");
    }
}

//ładowanie tagów dla notatki
void NoteApp::load_tags_of_note(QString name_note)
{
    if(database.isOpen())
    {
        QSqlQuery query("SELECT Tags.TagName FROM Tags, TagsNotes, Notes WHERE Notes.Id=TagsNotes.IdNote AND Tags.Id=TagsNotes.IdTag AND Notes.NoteName='" + name_note + "'");
        int idName = query.record().indexOf("TagName");

        if(ui->tags_from_note->layout() != NULL)
        {
            QLayoutItem* item;
            while((item = ui->tags_from_note->layout()->takeAt(0)) != NULL)
            {
                QWidget* widget = item->widget();
                delete widget;
                delete item;
            }
            delete ui->tags_from_note->layout();
        }

        QHBoxLayout *layout = new QHBoxLayout;
        QWidget *widget = ui->tags_from_note;
        layout->setAlignment(Qt::AlignLeft);
        widget->setLayout(layout);

        while(query.next())
        {
            QString name_ = query.value(idName).toString();

            QPushButton *button = new QPushButton();
            button->setText(name_);

            QMenu* menu = new QMenu();
            QAction* action = new QAction("Remove");
            menu->addAction(action);
            button->setMenu(menu);
            layout->addWidget(button);

            connect(action, &QAction::triggered, action, [=]{remove_tag_from_note(name_);});
        }
    }
    else return;
}

//dodanie tagu do notatki
void NoteApp::on_add_tag_to_note_clicked()
{
    if(openNote)
    {
        if(database.isOpen())
        {
            bool ok;
            QStringList items;

            QSqlQuery query("SELECT TagName FROM Tags WHERE Id NOT IN (SELECT IdTag FROM TagsNotes WHERE IdNote='" + QString::number(idOpenNote) + "')");
            int idName = query.record().indexOf("TagName");
            int count = 0;
            QString name;
            while(query.next())
            {
                items << query.value(idName).toString();
                count++;
            }

            if(count != 0)
            {
                name = QInputDialog::getItem(0, "Add tag to note",
                                             "Tag Name:",
                                             items,0, 0,
                                             &ok,
                                             Qt::CustomizeWindowHint
                                             | Qt::WindowTitleHint);
            }
            else
            {
                QMessageBox messageBox;
                messageBox.warning(0,"Warning","Open note have all tags or\nthere are no tags in the system!");
                return;
            }


            if(ok && !name.isEmpty())
            {
                QSqlQuery query_("SELECT Id FROM Tags WHERE TagName='" + name + "'");
                query_.exec();
                query_.last();
                QString idTag = query_.value(0).toString();

                QString idNote = QString::number(idOpenNote);
                QSqlQuery query_insert;
                query_insert.prepare("INSERT INTO TagsNotes (IdTag, IdNote) VALUES (:idTag, :idNote)");
                query_insert.bindValue(":idTag", idTag);
                query_insert.bindValue(":idNote", idNote);

                if(query_insert.exec()) load_tags_of_note(nameOpenNote);
            }
        }
    }
    else
    {
        QMessageBox messageBox;
        messageBox.warning(0,"Warning","Open note to add tag!");
        return;
    }
}
