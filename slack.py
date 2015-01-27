#!/usr/bin/env python

#############################################################################
#
# PyQt5 slack client
# Copyright (C) 2015 Reza Jelveh
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, see <http://www.gnu.org/licenses/>.
#
###########################################################################

from PyQt5.QtGui import QIcon, QPixmap, QKeySequence
from PyQt5.QtCore import (QFileInfo, QUrl, QSettings, QByteArray,
                          QObject, pyqtSlot, pyqtProperty)
from PyQt5.QtWidgets import (QAction, QApplication, QMainWindow,
                             QSystemTrayIcon, QMessageBox, QDialog,
                             QVBoxLayout, QMenu, QShortcut)
from PyQt5.QtNetwork import QNetworkProxyFactory, QNetworkCookieJar, QNetworkCookie
from PyQt5.QtWebKitWidgets import QWebPage, QWebView, QWebInspector
from PyQt5.QtWebKit import QWebSettings
from gi.repository import Notify
# from os import startfile
import subprocess

# from gi.repository import NM


class cookieJar(QNetworkCookieJar):
    def __init__(self, cookiesKey, parent=None):
        super(cookieJar, self).__init__(parent)

        self.mainWindow = parent
        self.cookiesKey = cookiesKey
        cookiesValue    = self.mainWindow.settings.value(self.cookiesKey)

        if cookiesValue:
            cookiesList = QNetworkCookie.parseCookies(cookiesValue)
            self.setAllCookies(cookiesList)

    def setCookiesFromUrl(self, cookieList, url):
        cookiesValue = self.mainWindow.settings.value(self.cookiesKey)
        cookiesArray = cookiesValue if cookiesValue else QByteArray()

        for cookie in cookieList:
            cookiesArray.append(cookie.toRawForm() + "\n")

        self.mainWindow.settings.setValue(self.cookiesKey, cookiesArray)

        return super(cookieJar, self).setCookiesFromUrl(cookieList, url)


class MainWindow(QMainWindow):
    def __init__(self, url):
        super(MainWindow, self).__init__()

        QShortcut(QKeySequence("Ctrl+Q"), self, QApplication.instance().quit)
        self.progress = 0
        self.settings = QSettings()

        QNetworkProxyFactory.setUseSystemConfiguration(True)

        self.view = QWebView(self)
        self.view.load(url)
        self.view.page().setLinkDelegationPolicy(QWebPage.DelegateExternalLinks)
        self.view.linkClicked.connect(self.clickLink)
        self.view.titleChanged.connect(self.adjustTitle)
        self.view.loadProgress.connect(self.setProgress)
        self.view.loadFinished.connect(self.finishLoading)
        cookiesKey = "cookies"
        self.cookieJar = cookieJar(cookiesKey, self)
        self.view.page().networkAccessManager().setCookieJar(self.cookieJar)

        settings = self.view.settings()
        settings.setAttribute(QWebSettings.NotificationsEnabled, False)

        self.setCentralWidget(self.view)
        self.createActions()
        self.createTrayIcon()
        self.trayIcon.activated.connect(self.iconActivated)
        # self.view.setSystemTrayIcon(self.trayIcon)

    def showInspector(self):
        dlg = QDialog(self)
        i = QWebInspector(self)
        vbox = QVBoxLayout()
        dlg.setLayout(vbox)
        dlg.setModal(False)
        dlg.show()
        dlg.raise_()
        dlg.activateWindow()
        dlg.resize(800, 400)
        settings = self.view.settings()
        settings.setAttribute(QWebSettings.DeveloperExtrasEnabled, True)
        i.setPage(self.view.page())
        i.setVisible(True)
        i.setEnabled(True)
        i.raise_()
        i.show()
        # it's important to add the widget after everything has been setup
        # otherwise it won't show up
        vbox.addWidget(i)
        self.view.triggerPageAction(QWebPage.InspectElement)

    def clickLink(self, url):
        # if sys.platform.startswith('linux'):
        subprocess.call(["xdg-open", url.toString()])
        # else:
        #     startfile(file)

    def adjustTitle(self):
        if 0 < self.progress < 100:
            self.setWindowTitle("%s (%s%%)" % (self.view.title(), self.progress))
        else:
            self.setWindowTitle(self.view.title())

    def setProgress(self, p):
        self.progress = p
        self.adjustTitle()

    def setPermissions(self):
        self.view.page().setFeaturePermission(self.view.page().mainFrame(), 0,
                QWebPage.PermissionGrantedByUser);

    def finishLoading(self):
        self.progress = 100
        self.adjustTitle()
        self.setPermissions()

    def createActions(self):
        self.minimizeAction = QAction("Mi&nimize", self, triggered=self.hide)
        self.restoreAction = QAction("&Restore", self,
                triggered=self.showMaximized)
        self.quitAction = QAction("&Quit", self, shortcut="Ctrl+Q",
                triggered=QApplication.instance().quit)

    def createTrayIcon(self):
        self.trayIconMenu = QMenu(self)
        self.trayIconMenu.addAction(self.minimizeAction)
        self.trayIconMenu.addAction(self.restoreAction)
        self.trayIconMenu.addSeparator()
        self.trayIconMenu.addAction(self.quitAction)
        info = QFileInfo(__file__)
        if info.isSymLink():
            _root = QFileInfo(info.symLinkTarget()).absolutePath()
        else:
            _root = info.absolutePath()

        icon = QIcon(QPixmap(_root + '/icons/icon_512x512.png'))

        self.trayIcon = QSystemTrayIcon(self)
        self.trayIcon.setIcon(icon)
        self.trayIcon.show()
        QApplication.setWindowIcon(icon)
        self.trayIcon.setContextMenu(self.trayIconMenu)

    def iconActivated(self, reason):
        if reason in (QSystemTrayIcon.Trigger, QSystemTrayIcon.DoubleClick):
            if self.isVisible():
                self.hide()
            else:
                self.showMaximized()


if __name__ == "__main__":
    # Notify.init ("Slack")
    # create Client object
    # client = NM.Client.new(None)

    # # get all connections
    # connections = client.get_connections()

    # # print the connections' details
    # for c in connections:
    #     print("=== %s : %s ===" % (c.get_id(), c.get_path()))
    #     c.for_each_setting_value(print_values, None)
    #     print("\n")

    import sys

    app = QApplication(sys.argv)

    if not QSystemTrayIcon.isSystemTrayAvailable():
        QMessageBox.critical(None, "Systray",
                "I couldn't detect any system tray on this system.")
        sys.exit(1)

    QApplication.setQuitOnLastWindowClosed(False)

    url = QUrl('http://saucelabs.slack.com')
    browser = MainWindow(url)
    browser.show()

    sys.exit(app.exec_())
