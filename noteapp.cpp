#include "noteapp.h"
#include "ui_noteapp.h"
#include "previewpage.h"

#include <QFontDatabase>
#include <QWebChannel>

NoteApp::NoteApp(QWidget *parent): QMainWindow(parent), ui(new Ui::NoteApp)
{
    ui->setupUi(this);

    ui->editor->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    ui->preview->setContextMenuPolicy(Qt::NoContextMenu);

    PreviewPage *page = new PreviewPage(this);
    ui->preview->setPage(page);
    ui->preview->resize(280, 1);

    connect(ui->editor, &QPlainTextEdit::textChanged, [this]() { m_content.setText(ui->editor->toPlainText()); });

    QWebChannel *channel = new QWebChannel(this);
    channel->registerObject(QStringLiteral("content"), &m_content);
    page->setWebChannel(channel);

    ui->preview->setUrl(QUrl("qrc:/index.html"));
}

NoteApp::~NoteApp()
{
    delete ui;
}
