#include "GlobalMediaKeysHook.hpp"

#include <QCoreApplication>

#if defined(PICAUDIO_ENABLE_DBUS)
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#endif

#if defined(_WIN32)
#include <QAbstractNativeEventFilter>
#include <windows.h>
#endif

namespace adapters {

#if defined(_WIN32)
class WinMediaKeyEventFilter : public QAbstractNativeEventFilter {
public:
  explicit WinMediaKeyEventFilter(GlobalMediaKeysHook *hook) : m_hook(hook) {}

  bool nativeEventFilter(const QByteArray &eventType, void *message,
                         qintptr *result) override {
    Q_UNUSED(eventType);
    Q_UNUSED(result);
    MSG *msg = static_cast<MSG *>(message);
    if (msg->message == WM_HOTKEY && m_hook && m_hook->isEnabled()) {
      switch (msg->wParam) {
      case 1001: // Play / Pause
        emit m_hook->playPauseRequested();
        return true;
      case 1002: // Next
        emit m_hook->nextRequested();
        return true;
      case 1003: // Previous
        emit m_hook->previousRequested();
        return true;
      case 1004: // Stop
        emit m_hook->stopRequested();
        return true;
      case 1005: // Mute
        emit m_hook->muteRequested();
        return true;
      case 1006: // Volume Up
        emit m_hook->volumeUpRequested();
        return true;
      case 1007: // Volume Down
        emit m_hook->volumeDownRequested();
        return true;
      default:
        break;
      }
    }
    return false;
  }

private:
  GlobalMediaKeysHook *m_hook{nullptr};
};
#endif

struct GlobalMediaKeysHook::Impl {
#if defined(_WIN32)
  WinMediaKeyEventFilter *eventFilter{nullptr};
#endif
  bool dbusConnected{false};
};

GlobalMediaKeysHook::GlobalMediaKeysHook(QObject *parent)
    : QObject(parent), m_impl(new Impl()) {}

GlobalMediaKeysHook::~GlobalMediaKeysHook() {
  unregisterHooks();
  delete m_impl;
}

bool GlobalMediaKeysHook::initialize() {
  if (m_enabled) {
    registerHooks();
  }
  return true;
}

void GlobalMediaKeysHook::setEnabled(bool enable) {
  if (m_enabled == enable) {
    return;
  }
  m_enabled = enable;
  if (m_enabled) {
    registerHooks();
  } else {
    unregisterHooks();
  }
}

void GlobalMediaKeysHook::registerHooks() {
  if (m_registered) {
    return;
  }

#if defined(PICAUDIO_ENABLE_DBUS)
  QDBusConnection bus = QDBusConnection::sessionBus();
  if (bus.isConnected()) {
    bus.connect(QStringLiteral("org.gnome.SettingsDaemon.MediaKeys"),
                QStringLiteral("/org/gnome/SettingsDaemon/MediaKeys"),
                QStringLiteral("org.gnome.SettingsDaemon.MediaKeys"),
                QStringLiteral("MediaPlayerKeyPressed"), this,
                SLOT(onGnomeMediaKeyPressed(QString, QString)));

    QDBusMessage grabMsg = QDBusMessage::createMethodCall(
        QStringLiteral("org.gnome.SettingsDaemon.MediaKeys"),
        QStringLiteral("/org/gnome/SettingsDaemon/MediaKeys"),
        QStringLiteral("org.gnome.SettingsDaemon.MediaKeys"),
        QStringLiteral("GrabMediaPlayerKeys"));
    grabMsg << QStringLiteral("picaudio") << static_cast<uint>(0);
    bus.call(grabMsg, QDBus::NoBlock);
    m_impl->dbusConnected = true;
  }
#endif

#if defined(_WIN32)
  if (!m_impl->eventFilter) {
    m_impl->eventFilter = new WinMediaKeyEventFilter(this);
    QCoreApplication::instance()->installNativeEventFilter(m_impl->eventFilter);
  }
  RegisterHotKey(nullptr, 1001, MOD_NOREPEAT, VK_MEDIA_PLAY_PAUSE);
  RegisterHotKey(nullptr, 1002, MOD_NOREPEAT, VK_MEDIA_NEXT_TRACK);
  RegisterHotKey(nullptr, 1003, MOD_NOREPEAT, VK_MEDIA_PREV_TRACK);
  RegisterHotKey(nullptr, 1004, MOD_NOREPEAT, VK_MEDIA_STOP);
  RegisterHotKey(nullptr, 1005, MOD_NOREPEAT, VK_VOLUME_MUTE);
  RegisterHotKey(nullptr, 1006, MOD_NOREPEAT, VK_VOLUME_UP);
  RegisterHotKey(nullptr, 1007, MOD_NOREPEAT, VK_VOLUME_DOWN);
#endif

  m_registered = true;
}

void GlobalMediaKeysHook::unregisterHooks() {
  if (!m_registered) {
    return;
  }

#if defined(PICAUDIO_ENABLE_DBUS)
  if (m_impl->dbusConnected) {
    QDBusConnection bus = QDBusConnection::sessionBus();
    bus.disconnect(QStringLiteral("org.gnome.SettingsDaemon.MediaKeys"),
                   QStringLiteral("/org/gnome/SettingsDaemon/MediaKeys"),
                   QStringLiteral("org.gnome.SettingsDaemon.MediaKeys"),
                   QStringLiteral("MediaPlayerKeyPressed"), this,
                   SLOT(onGnomeMediaKeyPressed(QString, QString)));

    QDBusMessage releaseMsg = QDBusMessage::createMethodCall(
        QStringLiteral("org.gnome.SettingsDaemon.MediaKeys"),
        QStringLiteral("/org/gnome/SettingsDaemon/MediaKeys"),
        QStringLiteral("org.gnome.SettingsDaemon.MediaKeys"),
        QStringLiteral("ReleaseMediaPlayerKeys"));
    releaseMsg << QStringLiteral("picaudio");
    bus.call(releaseMsg, QDBus::NoBlock);
    m_impl->dbusConnected = false;
  }
#endif

#if defined(_WIN32)
  for (int id = 1001; id <= 1007; ++id) {
    UnregisterHotKey(nullptr, id);
  }
  if (m_impl->eventFilter) {
    QCoreApplication::instance()->removeNativeEventFilter(m_impl->eventFilter);
    delete m_impl->eventFilter;
    m_impl->eventFilter = nullptr;
  }
#endif

  m_registered = false;
}

#if defined(PICAUDIO_ENABLE_DBUS)
void GlobalMediaKeysHook::onGnomeMediaKeyPressed(const QString &app,
                                                 const QString &key) {
  if (!m_enabled || app != QStringLiteral("picaudio")) {
    return;
  }

  if (key == QStringLiteral("Play") || key == QStringLiteral("Pause") ||
      key == QStringLiteral("PlayPause")) {
    emit playPauseRequested();
  } else if (key == QStringLiteral("Next")) {
    emit nextRequested();
  } else if (key == QStringLiteral("Previous")) {
    emit previousRequested();
  } else if (key == QStringLiteral("Stop")) {
    emit stopRequested();
  } else if (key == QStringLiteral("Mute")) {
    emit muteRequested();
  } else if (key == QStringLiteral("VolumeUp")) {
    emit volumeUpRequested();
  } else if (key == QStringLiteral("VolumeDown")) {
    emit volumeDownRequested();
  }
}
#endif

} // namespace adapters
