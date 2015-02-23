/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "browserapplication.h"
#include "mainwindow.h"
// #include "downloadmanager.h"
// #include "featurepermissionbar.h"
// #include "networkaccessmanager.h"
// #include "ui_passworddialog.h"
// #include "ui_proxy.h"
// #include "tabwidget.h"
#include "webview.h"

#include <qwebchannel.h>

#include <QtGui/QClipboard>
#include <QtNetwork/QAuthenticator>
#include <QtNetwork/QtNetwork>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtGui/QMouseEvent>

#if defined(QWEBENGINEPAGE_HITTESTCONTENT)
#include <QWebEngineHitTestResult>
#endif

#ifndef QT_NO_UITOOLS
#include <QtUiTools/QUiLoader>
#endif  //QT_NO_UITOOLS

#include <QtCore/QDebug>
#include <QtCore/QBuffer>


#include <QStringList>

#include <QDir>
#include <QDBusInterface>
#include <QRegularExpression>

class QTimer;

class ChatServer : public QObject
{
    Q_OBJECT

    // Q_PROPERTY(QString hello MEMBER m_text)


public:
    explicit ChatServer(QObject *parent = 0);
    virtual ~ChatServer();

public:
    //a user sends a message to all other users
    Q_INVOKABLE void someMethod(const QString& title, const QString& body);

private slots:
    void downloaded(QNetworkReply* reply);
    void notifClosed(quint32 id, quint32 reason);
    void notifClicked(quint32 id, const QString &actionKey);

signals:
    void notificationClosed();
    void notificationClicked();

private:
    void sendNotification(const QString &title, const QString &body);
    QString m_text;
    // const QWebNotificationData* m_data;
    quint32 m_uid;
};

void ChatServer::notifClosed(quint32 id, quint32 reason)
{
    Q_UNUSED(reason)

    if (m_uid == id) {
        emit notificationClosed();
    }
}

void ChatServer::downloaded(QNetworkReply *reply)
{
    QByteArray response = reply->readAll();
    reply->deleteLater();

    QImage image;
    image.loadFromData(response);
    // sendNotification(image);
}

void ChatServer::sendNotification(const QString &title, const QString &body)
{
    QString fileName = QDir::tempPath() + "/qtwebkit-notif.png";

    QRegularExpression re("([Rr]eza|fishman|kvm|syncup)");
    QRegularExpressionMatch match = re.match(body);
    if (!match.hasMatch()) {
        return;
    }

    if (QFile(fileName).exists()) {
        QFile(fileName).remove();
    }

    // if (image.isNull()) {
        QImage(":notifications/webkit.png").save(fileName, "PNG");
    // }
    // else {
    //     image.save(fileName, "PNG");
    // }

    QDBusInterface dbus("org.freedesktop.Notifications", "/org/freedesktop/Notifications",
                        "org.freedesktop.Notifications", QDBusConnection::sessionBus());
    QVariantList args;
    args.append(QLatin1String("qtwebkitplugins"));
    args.append(m_uid);
    args.append(fileName);
    args.append(title);
    args.append(body);
    args.append(QStringList());
    args.append(QVariantMap());
    args.append(5000);

    QDBusMessage message = dbus.callWithArgumentList(QDBus::Block, "Notify", args);
    QVariantList list = message.arguments();
    if (list.count() > 0) {
        m_uid = list.at(0).toInt();
    }

    QDBusConnection::sessionBus().connect("org.freedesktop.Notifications",
                                          "/org/freedesktop/Notifications",
                                          "org.freedesktop.Notifications",
                                          "NotificationClosed", this,
                                          SLOT(notifClosed(quint32, quint32)));

    QDBusConnection::sessionBus().connect("org.freedesktop.Notifications",
                                          "/org/freedesktop/Notifications",
                                          "org.freedesktop.Notifications",
                                          "ActionInvoked", this,
                                          SLOT(notifClicked(quint32, const QString&)));

}

void ChatServer::notifClicked(quint32 id, const QString &actionKey)
{
    Q_UNUSED(actionKey)

    if (m_uid == id) {
        emit notificationClicked();
    }
}


ChatServer::ChatServer(QObject *parent)
    : QObject(parent)
    // , m_text("helloworld")
{
}

ChatServer::~ChatServer()
{}

void ChatServer::someMethod(const QString& title, const QString& body) {
  sendNotification(title, body);
}

WebPage::WebPage(QObject *parent)
    : QWebEnginePage(parent)
    , m_keyboardModifiers(Qt::NoModifier)
    , m_pressedButtons(Qt::NoButton)
    , m_openInNewTab(false)
{
// #if defined(QWEBENGINEPAGE_SETNETWORKACCESSMANAGER)
//     setNetworkAccessManager(BrowserApplication::networkAccessManager());
// #endif
#if defined(QWEBENGINEPAGE_UNSUPPORTEDCONTENT)
    connect(this, SIGNAL(unsupportedContent(QNetworkReply*)),
            this, SLOT(handleUnsupportedContent(QNetworkReply*)));
#endif
    connect(this, SIGNAL(authenticationRequired(const QUrl &, QAuthenticator*)),
            SLOT(authenticationRequired(const QUrl &, QAuthenticator*)));
    connect(this, SIGNAL(proxyAuthenticationRequired(const QUrl &, QAuthenticator *, const QString &)),
            SLOT(proxyAuthenticationRequired(const QUrl &, QAuthenticator *, const QString &)));
}

MainWindow *WebPage::mainWindow()
{
    QObject *w = this->parent();
    while (w) {
        if (MainWindow *mw = qobject_cast<MainWindow*>(w))
            return mw;
        w = w->parent();
    }
    return BrowserApplication::instance()->mainWindow();
}

#if defined(QWEBENGINEPAGE_ACCEPTNAVIGATIONREQUEST)
bool WebPage::acceptNavigationRequest(QWebEngineFrame *frame, const QNetworkRequest &request, NavigationType type)
{
    // ctrl open in new tab
    // ctrl-shift open in new tab and select
    // ctrl-alt open in new window
    if (type == QWebEnginePage::NavigationTypeLinkClicked
        && (m_keyboardModifiers & Qt::ControlModifier
            || m_pressedButtons == Qt::MidButton)) {
        bool newWindow = (m_keyboardModifiers & Qt::AltModifier);
        WebView *webView;
        if (newWindow) {
            BrowserApplication::instance()->newMainWindow();
            MainWindow *newMainWindow = BrowserApplication::instance()->mainWindow();
            webView = newMainWindow->webView();
            newMainWindow->raise();
            newMainWindow->activateWindow();
            webView->setFocus();
        } else {
            bool selectNewTab = (m_keyboardModifiers & Qt::ShiftModifier);
            // webView = mainWindow()->tabWidget()->newTab(selectNewTab);
        }
        webView->load(request);
        m_keyboardModifiers = Qt::NoModifier;
        m_pressedButtons = Qt::NoButton;
        return false;
    }
    m_loadingUrl = request.url();
    emit loadingUrl(m_loadingUrl);
}
#endif

bool WebPage::certificateError(const QWebEngineCertificateError &error)
{
    if (error.isOverridable()) {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText(error.errorDescription());
        msgBox.setInformativeText(tr("If you wish so, you may continue with an unverified certificate. "
                                     "Accepting an unverified certificate means "
                                     "you may not be connected with the host you tried to connect to.\n"
                                     "Do you wish to override the security check and continue?"));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);
        return msgBox.exec() == QMessageBox::Yes;
    }
    QMessageBox::critical(view(), tr("Certificate Error"), error.errorDescription(), QMessageBox::Ok, QMessageBox::NoButton);
    return false;
}


#include "webview.moc"

QWebEnginePage *WebPage::createWindow(QWebEnginePage::WebWindowType type)
{
    if (type == QWebEnginePage::WebBrowserWindow) {
        BrowserApplication::instance()->newMainWindow();
        MainWindow *mainWindow = BrowserApplication::instance()->mainWindow();
        return mainWindow->webView()->page();
    }
}

#if !defined(QT_NO_UITOOLS)
QObject *WebPage::createPlugin(const QString &classId, const QUrl &url, const QStringList &paramNames, const QStringList &paramValues)
{
    Q_UNUSED(url);
    Q_UNUSED(paramNames);
    Q_UNUSED(paramValues);
    QUiLoader loader;
    return loader.createWidget(classId, view());
}
#endif // !defined(QT_NO_UITOOLS)

#if defined(QWEBENGINEPAGE_UNSUPPORTEDCONTENT)
void WebPage::handleUnsupportedContent(QNetworkReply *reply)
{
    QString errorString = reply->errorString();

    if (m_loadingUrl != reply->url()) {
        // sub resource of this page
        qWarning() << "Resource" << reply->url().toEncoded() << "has unknown Content-Type, will be ignored.";
        reply->deleteLater();
        return;
    }

    if (reply->error() == QNetworkReply::NoError && !reply->header(QNetworkRequest::ContentTypeHeader).isValid()) {
        errorString = "Unknown Content-Type";
    }

    QFile file(QLatin1String(":/notfound.html"));
    bool isOpened = file.open(QIODevice::ReadOnly);
    Q_ASSERT(isOpened);
    Q_UNUSED(isOpened)

    QString title = tr("Error loading page: %1").arg(reply->url().toString());
    QString html = QString(QLatin1String(file.readAll()))
                        .arg(title)
                        .arg(errorString)
                        .arg(reply->url().toString());

    QBuffer imageBuffer;
    imageBuffer.open(QBuffer::ReadWrite);
    QIcon icon = view()->style()->standardIcon(QStyle::SP_MessageBoxWarning, 0, view());
    QPixmap pixmap = icon.pixmap(QSize(32,32));
    if (pixmap.save(&imageBuffer, "PNG")) {
        html.replace(QLatin1String("IMAGE_BINARY_DATA_HERE"),
                     QString(QLatin1String(imageBuffer.buffer().toBase64())));
    }

    QList<QWebEngineFrame*> frames;
    frames.append(mainFrame());
    while (!frames.isEmpty()) {
        QWebEngineFrame *frame = frames.takeFirst();
        if (frame->url() == reply->url()) {
            frame->setHtml(html, reply->url());
            return;
        }
        QList<QWebEngineFrame *> children = frame->childFrames();
        foreach (QWebEngineFrame *frame, children)
            frames.append(frame);
    }
    if (m_loadingUrl == reply->url()) {
        mainFrame()->setHtml(html, reply->url());
    }
}
#endif

void WebPage::authenticationRequired(const QUrl &requestUrl, QAuthenticator *auth)
{
    MainWindow *mainWindow = BrowserApplication::instance()->mainWindow();

    QDialog dialog(mainWindow);
    dialog.setWindowFlags(Qt::Sheet);

    /*
    Ui::PasswordDialog passwordDialog;
    passwordDialog.setupUi(&dialog);

    passwordDialog.iconLabel->setText(QString());
    passwordDialog.iconLabel->setPixmap(mainWindow->style()->standardIcon(QStyle::SP_MessageBoxQuestion, 0, mainWindow).pixmap(32, 32));

    QString introMessage = tr("<qt>Enter username and password for \"%1\" at %2</qt>");
    introMessage = introMessage.arg(auth->realm()).arg(requestUrl.toString().toHtmlEscaped());
    passwordDialog.introLabel->setText(introMessage);
    passwordDialog.introLabel->setWordWrap(true);

    if (dialog.exec() == QDialog::Accepted) {
        auth->setUser(passwordDialog.userNameLineEdit->text());
        auth->setPassword(passwordDialog.passwordLineEdit->text());
    }
    */
}

void WebPage::proxyAuthenticationRequired(const QUrl &requestUrl, QAuthenticator *auth, const QString &proxyHost)
{
  /*
    Q_UNUSED(requestUrl);
    MainWindow *mainWindow = BrowserApplication::instance()->mainWindow();

    QDialog dialog(mainWindow);
    dialog.setWindowFlags(Qt::Sheet);

    Ui::ProxyDialog proxyDialog;
    proxyDialog.setupUi(&dialog);

    proxyDialog.iconLabel->setText(QString());
    proxyDialog.iconLabel->setPixmap(mainWindow->style()->standardIcon(QStyle::SP_MessageBoxQuestion, 0, mainWindow).pixmap(32, 32));

    QString introMessage = tr("<qt>Connect to proxy \"%1\" using:</qt>");
    introMessage = introMessage.arg(proxyHost.toHtmlEscaped());
    proxyDialog.introLabel->setText(introMessage);
    proxyDialog.introLabel->setWordWrap(true);

    if (dialog.exec() == QDialog::Accepted) {
        auth->setUser(proxyDialog.userNameLineEdit->text());
        auth->setPassword(proxyDialog.passwordLineEdit->text());
    }
    */
}

WebView::WebView(QWidget* parent)
    : QWebEngineView(parent)
    , m_progress(0)
    , m_page(new WebPage(this))
    , m_iconReply(0)
{
    setPage(m_page);
#if defined(QWEBENGINEPAGE_STATUSBARMESSAGE)
    // connect(page(), SIGNAL(statusBarMessage(QString)),
    //         SLOT(setStatusBarText(QString)));
#endif
    connect(this, SIGNAL(loadProgress(int)),
            this, SLOT(setProgress(int)));
    connect(this, SIGNAL(loadFinished(bool)),
            this, SLOT(loadFinished(bool)));
    connect(page(), SIGNAL(loadingUrl(QUrl)),
            this, SIGNAL(urlChanged(QUrl)));
    connect(page(), SIGNAL(iconUrlChanged(QUrl)),
            this, SLOT(onIconUrlChanged(QUrl)));
#if defined(QWEBENGINEPAGE_DOWNLOADREQUESTED)
    connect(page(), SIGNAL(downloadRequested(QNetworkRequest)),
            this, SLOT(downloadRequested(QNetworkRequest)));
#endif
    // connect(page(), &WebPage::featurePermissionRequested, this, &WebView::onFeaturePermissionRequested);
#if defined(QWEBENGINEPAGE_UNSUPPORTEDCONTENT)
    page()->setForwardUnsupportedContent(true);
#endif

}

void WebView::contextMenuEvent(QContextMenuEvent *event)
{
#if defined(QWEBENGINEPAGE_HITTESTCONTENT)
    QWebEngineHitTestResult r = page()->hitTestContent(event->pos());
    if (!r.linkUrl().isEmpty()) {
        QMenu menu(this);
        menu.addAction(pageAction(QWebEnginePage::OpenLinkInNewWindow));
        menu.addAction(tr("Open in New Tab"), this, SLOT(openLinkInNewTab()));
        menu.addSeparator();
        menu.addAction(pageAction(QWebEnginePage::DownloadLinkToDisk));
        // Add link to bookmarks...
        menu.addSeparator();
        menu.addAction(pageAction(QWebEnginePage::CopyLinkToClipboard));
        if (page()->settings()->testAttribute(QWebEngineSettings::DeveloperExtrasEnabled))
            menu.addAction(pageAction(QWebEnginePage::InspectElement));
        menu.exec(mapToGlobal(event->pos()));
        return;
    }
#endif
    QWebEngineView::contextMenuEvent(event);
}

void WebView::wheelEvent(QWheelEvent *event)
{
#if defined(QWEBENGINEPAGE_SETTEXTSIZEMULTIPLIER)
    if (QApplication::keyboardModifiers() & Qt::ControlModifier) {
        int numDegrees = event->delta() / 8;
        int numSteps = numDegrees / 15;
        setTextSizeMultiplier(textSizeMultiplier() + numSteps * 0.1);
        event->accept();
        return;
    }
#endif
    QWebEngineView::wheelEvent(event);
}

void WebView::openLinkInNewTab()
{
#if defined(QWEBENGINEPAGE_WEBACTION_OPENLINKINNEWWINDOW)
    m_page->m_openInNewTab = true;
    pageAction(QWebEnginePage::OpenLinkInNewWindow)->trigger();
#endif
}

void WebView::onFeaturePermissionRequested(const QUrl &securityOrigin, QWebEnginePage::Feature feature)
{
    // FeaturePermissionBar *permissionBar = new FeaturePermissionBar(this);
    // connect(permissionBar, &FeaturePermissionBar::featurePermissionProvided, page(), &QWebEnginePage::setFeaturePermission);

    // // Discard the bar on new loads (if we navigate away or reload).
    // connect(page(), &QWebEnginePage::loadStarted, permissionBar, &QObject::deleteLater);

    // permissionBar->requestPermission(securityOrigin, feature);
}

void WebView::setProgress(int progress)
{
  // const char * javascript = 
  //     "window.webkitNotifications = {};"
  //     "window.webkitNotifications.checkPermission = function(){ return 0; };"
  //     "window.webkitNotifications.requestPermission = function() { return 0; };"
  //     "window.webkitNotifications.createNotification = function() { alert('notification') };";
  // m_page->runJavaScript(javascript, [](const QVariant &result){ qDebug() << result; });
    m_progress = progress;
}

void WebView::loadFinished(bool success)
{
      // "new QWebChannel(yourTransport, function(channel) {"
      // "window.foo = channel.objects.foo;"
      // "console.log(foo.hello);"
      // "});"
      // "new QWebChannel(navigator.qtWebChannelTransport, function(channel) {"
      // "// all published objects are available in channel.objects under"
      // "// the identifier set in their attached WebChannel.id property"
      // "var foo = channel.objects.foo;"
      // "alert(foo.hello);"
      // "// invoke a method, and receive the return value asynchronously"
      // "foo.someMethod(\"bar\", function(ret) {"
      // "alert('hello');"
      // "});"
      // "});"
  qWarning() << this->title();
  this->webPage()->mainWindow()->setTitle(this->title());
  const char * javascript =
      "\"use strict\";function QObject(e,n,t){function o(e){if(!e||!e[\"__QObject*__\"]||void 0===e.id||void 0===e.data)return e;var n=e.id;if(t.objects[n])return t.objects[n];var o=new QObject(n,e.data,t);return o.destroyed.connect(function(){if(t.objects[n]===o){delete t.objects[n];var e=[];for(var a in o)e.push(a);for(var i in e)delete o[e[i]]}}),o}function a(e,n){var o=e[0],a=e[1];c[o]={connect:function(e){return\"function\"!=typeof e?void console.error(\"Bad callback given to connect to signal \"+o):(c.__objectSignals__[a]=c.__objectSignals__[a]||[],c.__objectSignals__[a].push(e),void(n||\"destroyed\"===o||t.exec({type:QWebChannelMessageTypes.connectToSignal,object:c.__id__,signal:a})))},disconnect:function(e){if(\"function\"!=typeof e)return void console.error(\"Bad callback given to disconnect from signal \"+o);c.__objectSignals__[a]=c.__objectSignals__[a]||[];var i=c.__objectSignals__[a].indexOf(e);return-1===i?void console.error(\"Cannot find connection of signal \"+o+\" to \"+e.name):(c.__objectSignals__[a].splice(i,1),void(n||0!==c.__objectSignals__[a].length||t.exec({type:QWebChannelMessageTypes.disconnectFromSignal,object:c.__id__,signal:a})))}}}function i(e,n){var t=c.__objectSignals__[e];t&&t.forEach(function(e){e.apply(e,n)})}function s(e){var n=e[0],a=e[1];c[n]=function(){for(var e,n=[],i=0;i<arguments.length;++i)\"function\"==typeof arguments[i]?e=arguments[i]:n.push(arguments[i]);t.exec({type:QWebChannelMessageTypes.invokeMethod,object:c.__id__,method:a,args:n},function(n){if(void 0!==n){var t=o(n);e&&e(t)}})}}function r(e){var n=e[0],o=e[1],i=e[2];c.__propertyCache__[n]=e[3],i&&(1===i[0]&&(i[0]=o+\"Changed\"),a(i,!0)),Object.defineProperty(c,o,{get:function(){var e=c.__propertyCache__[n];return void 0===e&&console.warn('Undefined value in property cache for property \"'+o+'\" in object '+c.__id__),e},set:function(e){return void 0===e?void console.warn(\"Property setter for \"+o+\" called with undefined value!\"):(c.__propertyCache__[n]=e,void t.exec({type:QWebChannelMessageTypes.setProperty,object:c.__id__,property:n,value:e}))}})}this.__id__=e,t.objects[e]=this,this.__objectSignals__={},this.__propertyCache__={};var c=this;this.propertyUpdate=function(e,n){for(var t in n){var o=n[t];c.__propertyCache__[t]=o}for(var a in e)i(a,e[a])},this.signalEmitted=function(e,n){i(e,n)},n.methods.forEach(s),n.properties.forEach(r),n.signals.forEach(function(e){a(e,!1)});for(var e in n.enums)c[e]=n.enums[e]}var QWebChannelMessageTypes={signal:1,propertyUpdate:2,init:3,idle:4,debug:5,invokeMethod:6,connectToSignal:7,disconnectFromSignal:8,setProperty:9,response:10},QWebChannel=function(e,n){if(\"object\"!=typeof e||\"function\"!=typeof e.send)return void console.error(\"The QWebChannel expects a transport object with a send function and onmessage callback property. Given is: transport: \"+typeof e+\", transport.send: \"+typeof e.send);var t=this;this.transport=e,this.send=function(e){\"string\"!=typeof e&&(e=JSON.stringify(e)),t.transport.send(e)},this.transport.onmessage=function(e){var n=e.data;switch(\"string\"==typeof n&&(n=JSON.parse(n)),n.type){case QWebChannelMessageTypes.signal:t.handleSignal(n);break;case QWebChannelMessageTypes.response:t.handleResponse(n);break;case QWebChannelMessageTypes.propertyUpdate:t.handlePropertyUpdate(n);break;case QWebChannelMessageTypes.init:t.handleInit(n);break;default:console.error(\"invalid message received:\",e.data)}},this.execCallbacks={},this.execId=0,this.exec=function(e,n){return n?(t.execId===Number.MAX_VALUE&&(t.execId=Number.MIN_VALUE),e.hasOwnProperty(\"id\")?void console.error(\"Cannot exec message with property id: \"+JSON.stringify(e)):(e.id=t.execId++,t.execCallbacks[e.id]=n,void t.send(e))):void t.send(e)},this.objects={},this.handleSignal=function(e){var n=t.objects[e.object];n?n.signalEmitted(e.signal,e.args):console.warn(\"Unhandled signal: \"+e.object+\"::\"+e.signal)},this.handleResponse=function(e){return e.hasOwnProperty(\"id\")?(t.execCallbacks[e.id](e.data),void delete t.execCallbacks[e.id]):void console.error(\"Invalid response message received: \",JSON.stringify(e))},this.handlePropertyUpdate=function(e){for(var n in e.data){var o=e.data[n],a=t.objects[o.object];a?a.propertyUpdate(o.signals,o.properties):console.warn(\"Unhandled property update: \"+o.object+\"::\"+o.signal)}t.exec({type:QWebChannelMessageTypes.idle})},this.initialized=!1,this.handleInit=function(e){if(!t.initialized){t.initialized=!0;for(var o in e.data)var a=e.data[o],i=new QObject(o,a,t);n&&n(t),t.exec({type:QWebChannelMessageTypes.idle})}},this.debug=function(e){t.send({type:QWebChannelMessageTypes.debug,data:e})},t.exec({type:QWebChannelMessageTypes.init})};\"object\"==typeof module&&(module.exports={QWebChannel:QWebChannel});"
      "new QWebChannel(qt.webChannelTransport, function(channel) {"
      "window.webkitNotifications = {};"
      "window.webkitNotifications.checkPermission = function(){ return 0; };"
      "window.webkitNotifications.requestPermission = function() { return 0; };"
      "window.webkitNotifications.createNotification = function(icon, title, body) { foo.someMethod(title,body) };"
      "var foo = channel.objects.foo;"
      "});";
  m_page->runJavaScript(javascript, [](const QVariant &result){ qDebug() << result; });
  ChatServer* chatserver = new ChatServer();
  QWebChannel *channel = new QWebChannel();
  channel->registerObject("foo", chatserver);

  m_page->setWebChannel(channel);

    if (success && 100 != m_progress) {
        qWarning() << "Received finished signal while progress is still:" << progress()
                   << "Url:" << url();
    }
    m_progress = 0;
}

void WebView::loadUrl(const QUrl &url)
{
    m_initialUrl = url;
    load(url);
}

QString WebView::lastStatusBarText() const
{
    return m_statusBarText;
}

QUrl WebView::url() const
{
    QUrl url = QWebEngineView::url();
    if (!url.isEmpty())
        return url;

    return m_initialUrl;
}

QIcon WebView::icon() const
{
    if (!m_icon.isNull())
        return m_icon;
    return BrowserApplication::instance()->defaultIcon();
}

void WebView::onIconUrlChanged(const QUrl &url)
{
    // QNetworkRequest iconRequest(url);
    // m_iconReply = BrowserApplication::networkAccessManager()->get(iconRequest);
    // m_iconReply->setParent(this);
    // connect(m_iconReply, SIGNAL(finished()), this, SLOT(iconLoaded()));
}

void WebView::iconLoaded()
{
    m_icon = QIcon();
    if (m_iconReply) {
        QByteArray data = m_iconReply->readAll();
        QPixmap pixmap;
        pixmap.loadFromData(data);
        m_icon.addPixmap(pixmap);
        m_iconReply->deleteLater();
        m_iconReply = 0;
    }
    emit iconChanged();
}

void WebView::mousePressEvent(QMouseEvent *event)
{
    m_page->m_pressedButtons = event->buttons();
    m_page->m_keyboardModifiers = event->modifiers();
    QWebEngineView::mousePressEvent(event);
}

void WebView::mouseReleaseEvent(QMouseEvent *event)
{
    QWebEngineView::mouseReleaseEvent(event);
    if (!event->isAccepted() && (m_page->m_pressedButtons & Qt::MidButton)) {
        QUrl url(QApplication::clipboard()->text(QClipboard::Selection));
        if (!url.isEmpty() && url.isValid() && !url.scheme().isEmpty()) {
            setUrl(url);
        }
    }
}

void WebView::setStatusBarText(const QString &string)
{
    m_statusBarText = string;
}

void WebView::downloadRequested(const QNetworkRequest &request)
{
    // BrowserApplication::downloadManager()->download(request);
}
