#include "mainwindow.h"
#include "webview.h"
#include "ui_mainwindow.h"
#include <QtWebEngine/QtWebEngine>
#include <QtWebEngineWidgets/QtWebEngineWidgets>

#include <QtCore/QObject>


QString readFile (const QString& filename)
{
  QFile file(filename);
  if (file.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    QTextStream stream(&file);
    return stream.readAll();
  }
  return "";
}

MainWindow::MainWindow(QWidget *parent, Qt::WindowFlags flags)
    : QMainWindow(parent, flags)
    , m_webView(new WebView(this))
{
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout;

    layout->setSpacing(0);
    layout->setMargin(0);
    layout->addWidget(qobject_cast<QWidget*>(this->webView()));
    centralWidget->setLayout(layout);
    setCentralWidget(qobject_cast<QWidget*>(this->webView()));
    webView()->setHtml(readFile(QLatin1String(":index.html")));

}

MainWindow::~MainWindow()
{
    delete m_webView;
}

WebView *MainWindow::webView()
{
    return m_webView;
}

void MainWindow::loadPage(const QString &page)
{
    QUrl url = QUrl::fromUserInput(page);
    WebView *currentWebView = webView();
    if (currentWebView) {
        currentWebView->loadUrl(url);
        // currentWebView->setFocus();
    }
}

void MainWindow::slotHome()
{
    QSettings settings;
    settings.beginGroup(QLatin1String("MainWindow"));
    // QString home = settings.value(QLatin1String("home"), QLatin1String(defaultHome)).toString();
    // loadPage(home);
}

void MainWindow::setTitle(const QString &title)
{
  setWindowTitle(title);
}
