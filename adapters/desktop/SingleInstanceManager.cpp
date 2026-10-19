#include "SingleInstanceManager.hpp"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace adapters {

SingleInstanceManager::SingleInstanceManager(QObject *parent)
    : QObject(parent), m_server(new QLocalServer(this)) {
  connect(m_server, &QLocalServer::newConnection, this,
          &SingleInstanceManager::onNewConnection);
}

SingleInstanceManager::~SingleInstanceManager() {
  if (m_server && m_server->isListening()) {
    m_server->close();
    QLocalServer::removeServer(socketServerName());
  }
}

QString SingleInstanceManager::socketServerName() {
  QString user = QString::fromLocal8Bit(qgetenv("USER"));
  if (user.isEmpty()) {
    user = QString::fromLocal8Bit(qgetenv("USERNAME"));
  }
  if (user.isEmpty()) {
    user = QStringLiteral("default");
  }
  return QStringLiteral("PicAudio_SingleInstance_%1").arg(user);
}

bool SingleInstanceManager::checkOrStartPrimary(const QStringList &args,
                                                const QString &workingDir) {
  QString serverName = socketServerName();
  QString effectiveWorkingDir =
      workingDir.isEmpty() ? QDir::currentPath() : workingDir;

  // Try connecting to existing primary instance
  QLocalSocket socket;
  socket.connectToServer(serverName);

  if (socket.waitForConnected(400)) {
    // Another instance is already running; forward arguments
    QJsonObject obj;
    QJsonArray argArr;
    for (const QString &arg : args) {
      argArr.append(arg);
    }
    obj.insert(QStringLiteral("args"), argArr);
    obj.insert(QStringLiteral("workingDir"), effectiveWorkingDir);

    QByteArray data = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    socket.write(data);
    socket.waitForBytesWritten(1000);
    socket.disconnectFromServer();
    return false; // Secondary instance, should exit cleanly
  }

  // No active primary instance found; become primary
  QLocalServer::removeServer(serverName);
  m_server->listen(serverName);
  return true;
}

CliCommand SingleInstanceManager::parseArguments(const QStringList &args,
                                                 const QString &workingDir) {
  CliCommand cmd;
  QString effectiveDir =
      workingDir.isEmpty() ? QDir::currentPath() : workingDir;

  bool enqueueMode = false;
  QStringList rawPaths;

  for (int i = 0; i < args.size(); ++i) {
    const QString &arg = args.at(i);

    // Skip application binary invocation argument
    if (i == 0 &&
        (arg.endsWith(QStringLiteral("picaudio"), Qt::CaseInsensitive) ||
         arg.endsWith(QStringLiteral("picaudio.exe"), Qt::CaseInsensitive) ||
         arg.endsWith(QStringLiteral("PicAudio"), Qt::CaseInsensitive) ||
         arg.endsWith(QStringLiteral("PicAudio.exe"), Qt::CaseInsensitive))) {
      continue;
    }

    if (arg == QStringLiteral("--enqueue") || arg == QStringLiteral("-e")) {
      enqueueMode = true;
    } else if (arg == QStringLiteral("--play")) {
      cmd.type = CliCommand::Type::Play;
    } else if (arg == QStringLiteral("--pause")) {
      cmd.type = CliCommand::Type::Pause;
    } else if (arg == QStringLiteral("--toggle")) {
      cmd.type = CliCommand::Type::Toggle;
    } else if (arg == QStringLiteral("--stop")) {
      cmd.type = CliCommand::Type::Stop;
    } else if (arg == QStringLiteral("--next")) {
      cmd.type = CliCommand::Type::Next;
    } else if (arg == QStringLiteral("--prev") ||
               arg == QStringLiteral("--previous")) {
      cmd.type = CliCommand::Type::Previous;
    } else if (arg == QStringLiteral("--raise") ||
               arg == QStringLiteral("--show")) {
      cmd.type = CliCommand::Type::RaiseWindow;
    } else if (arg == QStringLiteral("--volume") && i + 1 < args.size()) {
      bool ok = false;
      int volVal = args.at(i + 1).toInt(&ok);
      if (ok) {
        cmd.type = CliCommand::Type::SetVolume;
        cmd.volume = static_cast<float>(std::clamp(volVal, 0, 100)) / 100.0f;
        ++i;
      }
    } else if (!arg.startsWith(QStringLiteral("-"))) {
      // Positional path argument
      rawPaths.append(arg);
    }
  }

  // Resolve positional paths
  if (!rawPaths.isEmpty()) {
    QStringList resolvedPaths;
    for (const QString &raw : rawPaths) {
      if (raw.contains(QStringLiteral("://"))) {
        resolvedPaths.append(raw);
      } else {
        QFileInfo fi(QDir(effectiveDir), raw);
        resolvedPaths.append(fi.exists() ? fi.absoluteFilePath() : raw);
      }
    }
    cmd.paths = resolvedPaths;
    if (cmd.type == CliCommand::Type::None) {
      cmd.type = enqueueMode ? CliCommand::Type::EnqueueFiles
                             : CliCommand::Type::PlayFiles;
    }
  }

  return cmd;
}

void SingleInstanceManager::onNewConnection() {
  while (m_server->hasPendingConnections()) {
    QLocalSocket *socket = m_server->nextPendingConnection();
    connect(socket, &QLocalSocket::readyRead, this,
            &SingleInstanceManager::onSocketReadyRead);
    connect(socket, &QLocalSocket::disconnected, socket,
            &QLocalSocket::deleteLater);
  }
}

void SingleInstanceManager::onSocketReadyRead() {
  auto *socket = qobject_cast<QLocalSocket *>(sender());
  if (!socket) {
    return;
  }

  QByteArray data = socket->readAll();
  QJsonDocument doc = QJsonDocument::fromJson(data);
  if (!doc.isObject()) {
    return;
  }

  QJsonObject obj = doc.object();
  QString workingDir = obj.value(QStringLiteral("workingDir")).toString();
  QJsonArray argArr = obj.value(QStringLiteral("args")).toArray();

  QStringList args;
  for (const auto &val : argArr) {
    args.append(val.toString());
  }

  CliCommand cmd = parseArguments(args, workingDir);
  emit commandReceived(cmd);
  if (cmd.raiseWindow) {
    emit raiseRequested();
  }
}

} // namespace adapters
