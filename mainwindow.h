#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class WebView;
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    WebView *webView();
    void setTitle(const QString &title);

public slots:
    void loadPage(const QString &url);
    void slotHome();

private:
    Ui::MainWindow *ui;
    WebView *m_webView;
};

#endif // MAINWINDOW_H
