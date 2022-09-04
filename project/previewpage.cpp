#include "previewpage.h"

#include <QDesktopServices>

bool PreviewPage::acceptNavigationRequest(const QUrl &url, QWebEnginePage::NavigationType, bool)
{
    //dopuszczamy tylko inex.html
    if (url.scheme() == QString("qrc"))
    {
        return true;
    }
    else
    {
        QDesktopServices::openUrl(url);
        return false;
    }
}
