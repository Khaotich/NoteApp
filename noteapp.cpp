#include "noteapp.h"
#include "ui_noteapp.h"
#include "previewpage.h"

#include <QFile>
#include <QFileDialog>
#include <QFontDatabase>
#include <QMessageBox>
#include <QTextStream>
#include <QWebChannel>

#include <windows.h>


//-------------------------------------------------------------------------------------------//
//----------------------------Inizjalizacja aplikacji QT-------------------------------------//


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
}

NoteApp::~NoteApp()
{
    delete ui;
}


//-------------------------------------------------------------------------------------------//
//---------------------------------Obsługa plików--------------------------------------------//


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

bool NoteApp::isModified() const
{
    return ui->editor->document()->isModified();
}

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

void NoteApp::onFileSaveAs()
{
    QString path = QFileDialog::getSaveFileName(this, tr("Save MarkDown File"), "", tr("MarkDown File (*.md *.markdown)"));
    if(path.isEmpty()) return;
    m_filePath = path;
    onFileSave();
}

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

//przycisk emoji
void NoteApp::on_button_emoji_clicked()
{
    return;
}

