#pragma once

#include <QLocalServer>
#include <QLocalSocket>
#include <QObject>
#include <QString>
#include <QStringList>

namespace adapters {

/**
 * @brief Parsed command line action or remote IPC instruction.
 */
struct CliCommand {
  enum class Type {
    None,
    PlayFiles,
    EnqueueFiles,
    Play,
    Pause,
    Toggle,
    Stop,
    Next,
    Previous,
    SetVolume,
    RaiseWindow
  };

  Type type{Type::None};
  QStringList paths;
  float volume{1.0f};
  bool raiseWindow{true};
};

/**
 * @brief Single-instance enforcement and IPC argument forwarding manager.
 */
class SingleInstanceManager : public QObject {
  Q_OBJECT

public:
  explicit SingleInstanceManager(QObject *parent = nullptr);
  ~SingleInstanceManager() override;

  /**
   * @brief Attempts to register as the primary single instance.
   * @param args Command-line arguments of this invocation.
   * @param workingDir Current working directory for resolving relative paths.
   * @return True if this is the primary instance; false if forwarded to running
   * instance.
   */
  bool checkOrStartPrimary(const QStringList &args,
                           const QString &workingDir = QString());

  /**
   * @brief Parses command line arguments into a structured CliCommand.
   * @param args Arguments list (excluding or including binary name).
   * @param workingDir Working directory used to resolve relative paths.
   */
  static CliCommand parseArguments(const QStringList &args,
                                   const QString &workingDir = QString());

  /**
   * @brief Returns the unique socket / named pipe name for this user session.
   */
  static QString socketServerName();

signals:
  void commandReceived(const adapters::CliCommand &cmd);
  void raiseRequested();

private slots:
  void onNewConnection();
  void onSocketReadyRead();

private:
  QLocalServer *m_server{nullptr};
};

} // namespace adapters
