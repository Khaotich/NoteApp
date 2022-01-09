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
#include <QtSql/QSqlDatabase>
#include <QInputDialog>

#include <Windows.h>


//-------------------------------------------------------------------------------------------//
//----------------------------Inicjalizacja aplikacji QT-------------------------------------//


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
    connect(ui->actionNew, &QAction::triggered, this, &NoteApp::onFileNew);
    connect(ui->actionOpen, &QAction::triggered, this, &NoteApp::onFileOpen);
    connect(ui->actionSave, &QAction::triggered, this, &NoteApp::onFileSave);
    connect(ui->actionSaveAs, &QAction::triggered, this, &NoteApp::onFileSaveAs);
    connect(ui->actionExit, &QAction::triggered, this, &NoteApp::onExit);
    connect(ui->editor->document(), &QTextDocument::modificationChanged,
            ui->actionSave, &QAction::setEnabled);
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


}

NoteApp::~NoteApp()
{
    delete ui;
}


//-------------------------------------------------------------------------------------------//
//---------------------------------Obsługa plików--------------------------------------------//

//otwarcie pliku
void NoteApp::openFile(const QString &path)
{
    QFile f(path);
    if(!f.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(this, windowTitle(),tr("Could not open file %1: %2").arg(QDir::toNativeSeparators(path), f.errorString()));
        return;
    }

    m_filePath = path;
    ui->editor->setPlainText(f.readAll());
}

//sprawdzenie czy plik się zmienił
bool NoteApp::isModified() const
{
    return ui->editor->document()->isModified();
}

//nowy plik
void NoteApp::onFileNew()
{
    if(isModified())
    {
        QMessageBox::StandardButton button = QMessageBox::question(this, windowTitle(), tr("You have unsaved changes. Do you want to create a new document anyway?"));
        if(button != QMessageBox::Yes) return;
    }

    m_filePath.clear();
    ui->editor->setPlainText(tr("## New document"));
    ui->editor->document()->setModified(false);
}

//otwarcie pliku
void NoteApp::onFileOpen()
{
    if(isModified())
    {
        QMessageBox::StandardButton button = QMessageBox::question(this, windowTitle(), tr("You have unsaved changes. Do you want to open a new document anyway?"));
        if(button != QMessageBox::Yes) return;
    }

    QString path = QFileDialog::getOpenFileName(this, tr("Open MarkDown File"), "", tr("MarkDown File (*.md)"));
    if(path.isEmpty()) return;

    openFile(path);
}

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

//wyjście bez zapisywania zmian
void NoteApp::onExit()
{
    if(isModified())
    {
        QMessageBox::StandardButton button = QMessageBox::question(this, windowTitle(), tr("You have unsaved changes. Do you want to exit anyway?"));
        if(button != QMessageBox::Yes) return;
    }
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

//nadpisujemy
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
//--------------------------------Obsługa bazy danych----------------------------------------//


//przycisk dodanie notatnika
void NoteApp::on_button_add_notebook_clicked()
{
    bool ok;
    QString notebook_name = QInputDialog::getText(0, "Input dialog",
                                                  "Notenook Name:", QLineEdit::Normal,
                                                   "", &ok);;

    if(ok && !notebook_name.isEmpty())
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
        db.setDatabaseName("database.db");
        if(db.open()) qDebug() << "work";
        else qDebug() << "fail";
    }
}

