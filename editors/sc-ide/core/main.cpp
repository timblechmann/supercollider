/*
    SuperCollider Qt IDE
    Copyright (c) 2012 Jakob Leben & Tim Blechmann
    http://www.audiosynth.com

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include "main.hpp"
#include "settings/manager.hpp"
#include "session_manager.hpp"
#include "util/standard_dirs.hpp"
#include "../widgets/main_window.hpp"
#include "../widgets/help_browser.hpp"
#include "../widgets/lookup_dialog.hpp"
#include "../widgets/code_editor/highlighter.hpp"
#include "../widgets/style/style.hpp"
#include "../../../QtCollider/hacks/hacks_mac.hpp"

#include "yaml-cpp/node.h"
#include "yaml-cpp/parser.h"

#include <QAction>
#include <QApplication>
#include <QBuffer>
#include <QDataStream>
#include <QDir>
#include <QFileOpenEvent>
#include <QLibraryInfo>
#include <QStyleFactory>
#include <QTranslator>
#include <QDebug>

using namespace ScIDE;

int main( int argc, char *argv[] )
{
    QApplication app(argc, argv);

    QStringList arguments (QApplication::arguments());
    arguments.pop_front(); // application path

    // Pass files to existing instance and quit

    SingleInstanceGuard guard;
    if (guard.tryConnect(arguments))
        return 0;

    // Set up translations

    QTranslator qtTranslator;
    qtTranslator.load("qt_" + QLocale::system().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    app.installTranslator(&qtTranslator);

    QString ideTranslationPath = standardDirectory(ScResourceDir) + "/translations";

    bool translationLoaded;

    // Load fallback translator that only handles plural forms in English
    QTranslator fallbackTranslator;
    translationLoaded = fallbackTranslator.load( "scide", ideTranslationPath );
    app.installTranslator(&fallbackTranslator);
    if (!translationLoaded)
        qWarning("scide warning: Failed to load fallback translation file.");

    // Load translator for locale
    QString ideTranslationFile = "scide_" + QLocale::system().name();
    QTranslator scideTranslator;
    scideTranslator.load( ideTranslationFile, ideTranslationPath );
    app.installTranslator(&scideTranslator);

    // Set up style

    app.setStyle( new ScIDE::Style(app.style()) );



    // Go...

    Main * main = Main::instance();

    MainWindow *win = new MainWindow(main);

    // NOTE: load session after GUI is created, so that GUI can respond
    Settings::Manager *settings = main->settings();
    SessionManager *sessions = main->sessionManager();

    // NOTE: window has to be shown before restoring its geometry,
    // or else restoring maximized state will fail, if it has ever before
    // been saved un-maximized.
    win->show();

    QString startSessionName = settings->value("IDE/startWithSession").toString();
    if (startSessionName == "last") {
        QString lastSession = sessions->lastSession();
        if (!lastSession.isEmpty()) {
            sessions->openSession(lastSession);
        }
    }
    else if (!startSessionName.isEmpty()) {
        sessions->openSession(startSessionName);
    }

    if (!sessions->currentSession()) {
        win->restoreWindowState();
        sessions->newSession();
    }

    foreach (QString argument, arguments) {
        main->documentManager()->open(argument);
    }

    win->restoreDocuments();

    bool startInterpreter = settings->value("IDE/interpreter/autoStart").toBool();
    if (startInterpreter)
        main->scProcess()->startLanguage();

    return app.exec();
}


bool SingleInstanceGuard::tryConnect(QStringList const & arguments)
{
    const int maxNumberOfInstances = 128;
    if (!arguments.empty()) {
        for (int socketID = 0; socketID != maxNumberOfInstances; ++socketID) {
            QString serverName = QString("SuperColliderIDE_Singleton_%1").arg(socketID);
            QSharedPointer<QLocalSocket> socket (new QLocalSocket(this));
            socket->connectToServer(serverName);

            QStringList canonicalArguments;
            foreach (QString path, arguments) {
                QFileInfo info(path);
                canonicalArguments << info.canonicalFilePath();
            }

            if (socket->waitForConnected(200)) {
                QDataStream stream(socket.data());
                stream.setVersion(QDataStream::Qt_4_6);

                stream << QString("open");
                stream << canonicalArguments;
                if (!socket->waitForBytesWritten(300))
                    qWarning("SingleInstanceGuard: writing data to another IDE instance timed out");

                return true;
            }
        }
    }

    mIpcServer = new QLocalServer(this);
    for (int socketID = 0; socketID != maxNumberOfInstances; ++socketID) {
        QString serverName = QString("SuperColliderIDE_Singleton_%1").arg(socketID);

        bool listening = mIpcServer->listen(serverName);
        if (listening) {
            connect(mIpcServer, SIGNAL(newConnection()), this, SLOT(onNewIpcConnection()));
            return false;
        }
    }
    return false;
}

void SingleInstanceGuard::onIpcData()
{
    QByteArray ipcData = mIpcSocket->readAll();

    QBuffer receivedData ( &ipcData );
    receivedData.open ( QIODevice::ReadOnly );

    QDataStream in ( &receivedData );
    in.setVersion ( QDataStream::Qt_4_6 );
    QString id;
    in >> id;
    if ( in.status() != QDataStream::Ok )
        return;

    QStringList message;
    in >> message;
    if ( in.status() != QDataStream::Ok )
        return;

    if (id == QString("open")) {
        foreach (QString path, message)
            Main::documentManager()->open(path);
    }
}


static inline QString getSettingsFile()
{
    return standardDirectory(ScConfigUserDir) + "/sc_ide_conf.yaml";
}

// NOTE: mSettings must be the first to initialize,
// because other members use it!

Main::Main(void) :
    mSettings( new Settings::Manager( getSettingsFile(), this ) ),
    mScProcess( new ScProcess(mSettings, this) ),
    mScServer( new ScServer(mScProcess, mSettings, this) ),
    mDocManager( new DocumentManager(this, mSettings) ),
    mSessionManager( new SessionManager(mDocManager, this) )
{
    new SyntaxHighlighterGlobals(this, mSettings);

    connect(mScProcess, SIGNAL(response(QString,QString)),
            mDocManager, SLOT(handleScLangMessage(QString,QString)));

    setDarkPalette();

    qApp->installEventFilter(this);
    qApp->installNativeEventFilter(this);
}

void Main::quit() {
    mSessionManager->saveSession();
    storeSettings();
    mScProcess->stopLanguage();
    QApplication::quit();
}

bool Main::eventFilter(QObject *object, QEvent *event)
{
    switch (event->type()) {
    case QEvent::FileOpen:
    {
        // open the file dragged onto the application icon on Mac
        QFileOpenEvent *openEvent = static_cast<QFileOpenEvent*>(event);
        mDocManager->open(openEvent->file());
        return true;
    }

    case QEvent::MouseMove:
        QApplication::restoreOverrideCursor();
        break;

    default:
        break;
    }

    return QObject::eventFilter(object, event);
}

bool Main::nativeEventFilter(const QByteArray &, void * message, long *)
{
    bool result = false;

#ifdef Q_OS_MAC
    if (QtCollider::Mac::IsCmdPeriodKeyDown(reinterpret_cast<void *>(message)))
    {
//        QKeyEvent event(QEvent::KeyPress, Qt::Key_Period, Qt::ControlModifier, ".");
//        QApplication::sendEvent(this, &event);
        mScProcess->stopMain(); // we completely bypass the shortcut handling
        result = true;
    }
    else if (QtCollider::Mac::IsCmdPeriodKeyUp(reinterpret_cast<void *>(message)))
    {
        result = true;
    }
#endif 
  
    return result;
}

void Main::setDarkPalette()
{
    qApp->setStyle(QStyleFactory::create("Fusion"));

    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window,        QColor(20, 20, 20));
    darkPalette.setColor(QPalette::WindowText,    Qt::white);
    darkPalette.setColor(QPalette::Base,          QColor(10, 10, 10));
    darkPalette.setColor(QPalette::AlternateBase, QColor(15, 15, 15));
    darkPalette.setColor(QPalette::ToolTipBase,   QColor(30, 30, 30));
    darkPalette.setColor(QPalette::ToolTipText,   Qt::white);
    darkPalette.setColor(QPalette::Text,          Qt::white);
    darkPalette.setColor(QPalette::Button,        QColor(22, 22, 22));
    darkPalette.setColor(QPalette::ButtonText,    Qt::white);
    darkPalette.setColor(QPalette::BrightText,    Qt::red);
    darkPalette.setColor(QPalette::Link,          QColor(42, 130, 218));

    darkPalette.setColor(QPalette::Highlight,     QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);

    darkPalette.setColor(QPalette::Mid, QColor(27, 27, 27));

    qApp->setPalette(darkPalette);
}


bool Main::openDocumentation(const QString & string)
{
    QString symbol = string.trimmed();
    if (symbol.isEmpty())
        return false;

    HelpBrowserDocklet *helpDock = MainWindow::instance()->helpBrowserDocklet();
    helpDock->browser()->gotoHelpFor(symbol);
    helpDock->focus();
    return true;
}

bool Main::openDocumentationForMethod(const QString & className, const QString & methodName)
{
    HelpBrowserDocklet *helpDock = MainWindow::instance()->helpBrowserDocklet();
    helpDock->browser()->gotoHelpForMethod(className, methodName);
    helpDock->focus();
    return true;
}

void Main::openDefinition(const QString &string, QWidget * parent)
{
    QString definitionString = string.trimmed();

    LookupDialog dialog(parent);
    if (!definitionString.isEmpty())
        dialog.query(definitionString);
    dialog.exec();
}

void Main::openCommandLine(const QString &string)
{
    MainWindow::instance()->showCmdLine(string);
}

void Main::findReferences(const QString &string, QWidget * parent)
{
    QString definitionString = string.trimmed();

    ReferencesDialog dialog(parent);
    if (!definitionString.isEmpty())
        dialog.query(definitionString);
    dialog.exec();
}

