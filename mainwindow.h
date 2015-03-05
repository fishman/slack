#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QMainWindow>

class WebView;
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0, Qt::WindowFlags flags = 0);
    ~MainWindow();

    WebView *webView();
    void setTitle(const QString &title);
    WebView *createBlankView();
    void clearBlankView();

public slots:
    void loadPage(const QString &url);
    void slotHome();

private:
    WebView *m_webView;
    WebView *m_blankView;
};

#endif // MAINWINDOW_H
