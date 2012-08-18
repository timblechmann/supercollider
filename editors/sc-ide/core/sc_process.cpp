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

#include <QCoreApplication>
#include <QBuffer>

#include "SC_DirUtils.h"

#include "sc_introspection.hpp"
#include "sc_process.hpp"
#include "sc_server.hpp"
#include "main.hpp"
#include "settings/manager.hpp"

#include "yaml-cpp/node.h"
#include "yaml-cpp/parser.h"

namespace ScIDE {

SCProcess::SCProcess( Main *parent ):
    QProcess( parent ),
    mIpcServer( new QLocalServer(this) ),
    mIpcServerName("SCIde_" + QString::number(QCoreApplication::applicationPid()))
{
    mIntrospectionParser = new ScIntrospectionParser( parent->scResponder(), this );
    mIntrospectionParser->start();

    prepareActions(parent);

    connect(this, SIGNAL( readyRead() ),
            this, SLOT( onReadyRead() ));
    connect(mIpcServer, SIGNAL(newConnection()), this, SLOT(onNewIpcConnection()));
    connect(mIntrospectionParser, SIGNAL(done(ScLanguage::Introspection*)),
            this, SLOT(swapIntrospection(ScLanguage::Introspection*)));
}

void SCProcess::prepareActions(Main * main)
{
    QAction * action;
    mActions[StartSCLang] = action = new QAction(
        QIcon::fromTheme("system-run"), tr("Start SCLang"), this);
    connect(action, SIGNAL(triggered()), this, SLOT(start()) );

    mActions[RecompileClassLibrary] = action = new QAction(
        QIcon::fromTheme("system-reboot"), tr("Recompile Class Library"), this);
    connect(action, SIGNAL(triggered()), this, SLOT(recompileClassLibrary()) );

    mActions[StopSCLang] = action = new QAction(
        QIcon::fromTheme("system-shutdown"), tr("Stop SCLang"), this);
    connect(action, SIGNAL(triggered()), this, SLOT(stopLanguage()) );

    mActions[RunMain] = action = new QAction(
        QIcon::fromTheme("media-playback-start"), tr("Run Main"), this);
    connect(action, SIGNAL(triggered()), this, SLOT(runMain()));

    mActions[StopMain] = action = new QAction(
        QIcon::fromTheme("process-stop"), tr("Stop Main"), this);
    action->setShortcut(tr("Ctrl+.", "Stop Main (a.k.a. cmd-period)"));
    connect(action, SIGNAL(triggered()), this, SLOT(stopMain()));

    Settings::Manager *settings = main->settings();
    for (int i = 0; i < SCProcessActionCount; ++i)
        settings->addAction( mActions[i] );
}

void SCProcess::start (void)
{
    if (state() != QProcess::NotRunning) {
        statusMessage("Interpreter is already running.");
        return;
    }

    Settings::Manager *settings = Main::instance()->settings();
    settings->beginGroup("IDE/interpreter");

    QString workingDirectory = settings->value("runtimeDir").toString();
    QString configFile = settings->value("configFile").toString();

    settings->endGroup();

    QString sclangCommand;
#ifdef Q_OS_MAC
    char resourcePath[PATH_MAX];
    sc_GetResourceDirectory(resourcePath, PATH_MAX);

    sclangCommand = QString(resourcePath) + "/sclang.app"
#else
    sclangCommand = "sclang";
#endif

    QStringList sclangArguments;
    if(!configFile.isEmpty())
        sclangArguments << "-l" << configFile;
    sclangArguments << "-i" << "scqt";

    if(!workingDirectory.isEmpty())
        setWorkingDirectory(workingDirectory);

    QProcess::start(sclangCommand, sclangArguments);
    bool processStarted = QProcess::waitForStarted();
    if (!processStarted) {
        emit statusMessage("Could not start interpreter!");
    } else
        onSclangStart();
}


void SCProcess::onIpcData()
{
    mIpcData.append(mIpcSocket->readAll());

    QBuffer receivedData ( &mIpcData );
    receivedData.open ( QIODevice::ReadOnly );

    QDataStream in ( &receivedData );
    in.setVersion ( QDataStream::Qt_4_6 );
    QString id, message;
    in >> id;
    if ( in.status() != QDataStream::Ok )
        return;

    in >> message;
    if ( in.status() != QDataStream::Ok )
        return;

    mIpcData.remove ( 0, receivedData.pos() );

    emit response(id, message);
}

void ScResponder::onResponse( const QString & selector, const QString & data )
{
    static QString defaultServerRunningChangedSymbol("defaultServerRunningChanged");
    static QString introspectionSymbol("introspection");

    if (selector == defaultServerRunningChangedSymbol) {
        std::stringstream stream;
        stream << data.toStdString();
        YAML::Parser parser(stream);

        YAML::Node doc;

        bool serverRunningState;
        std::string hostName;
        int port;

        while(parser.GetNextDocument(doc)) {
            assert(doc.Type() == YAML::NodeType::Sequence);

            bool success = doc[0].Read(serverRunningState);
            if (!success) return; // LATER: report error?

            success = doc[1].Read(hostName);
            if (!success) return; // LATER: report error?

            success = doc[2].Read(port);
            if (!success) return; // LATER: report error?
        }

        emit serverRunningChanged (serverRunningState, QString(hostName.c_str()), port);
        return;
    }
    else if (selector == introspectionSymbol) {
        emit newIntrospectionData(data);
        return;
    }
}

} // namespace ScIDE
