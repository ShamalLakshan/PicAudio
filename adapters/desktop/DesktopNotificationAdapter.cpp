#include "DesktopNotificationAdapter.hpp"

#include <QFileInfo>
#include <algorithm>

#if defined(PICAUDIO_ENABLE_DBUS)
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>
#endif

namespace adapters {

DesktopNotificationAdapter::DesktopNotificationAdapter(QObject *parent)
    : QObject(parent) {}

void DesktopNotificationAdapter::initialize(QSystemTrayIcon *trayIcon) {
  m_trayIcon = trayIcon;

#if defined(PICAUDIO_ENABLE_DBUS)
  QDBusConnection bus = QDBusConnection::sessionBus();
  if (bus.isConnected()) {
    // Listen for notification action button clicks
    bus.connect(QStringLiteral("org.freedesktop.Notifications"),
                QStringLiteral("/org/freedesktop/Notifications"),
                QStringLiteral("org.freedesktop.Notifications"),
                QStringLiteral("ActionInvoked"), this,
                SLOT(onActionInvoked(uint, QString)));

    // Verify presence of notification server
    QDBusInterface notifyIface(QStringLiteral("org.freedesktop.Notifications"),
                               QStringLiteral("/org/freedesktop/Notifications"),
                               QStringLiteral("org.freedesktop.Notifications"),
                               bus);
    m_dbusNotificationsAvailable = notifyIface.isValid();
  }
#endif
}

void DesktopNotificationAdapter::setDurationSec(int seconds) {
  m_durationSec = std::clamp(seconds, 2, 10);
}

bool DesktopNotificationAdapter::shouldSuppressNotification() const {
  if (!m_enabled) {
    return true;
  }
  if (m_suppressWhenFocused && m_isWindowFocused) {
    return true;
  }
  if (m_isDndActive) {
    return true;
  }
  return false;
}

void DesktopNotificationAdapter::showTrackNotification(
    const QString &title, const QString &artist, const QString &album,
    const QString &audioSpecs, const QString &artFilePath) {
  if (shouldSuppressNotification()) {
    return;
  }

  QString summary = title.isEmpty() ? QStringLiteral("Now Playing") : title;
  QString body;
  if (!artist.isEmpty() && !album.isEmpty()) {
    body = QStringLiteral("%1 — %2").arg(artist, album);
  } else if (!artist.isEmpty()) {
    body = artist;
  } else if (!album.isEmpty()) {
    body = album;
  }

  if (!audioSpecs.isEmpty()) {
    if (!body.isEmpty()) {
      body += QStringLiteral("\n");
    }
    body += audioSpecs;
  }

#if defined(PICAUDIO_ENABLE_DBUS)
  if (m_dbusNotificationsAvailable) {
    QDBusMessage msg = QDBusMessage::createMethodCall(
        QStringLiteral("org.freedesktop.Notifications"),
        QStringLiteral("/org/freedesktop/Notifications"),
        QStringLiteral("org.freedesktop.Notifications"),
        QStringLiteral("Notify"));

    QStringList actions{QStringLiteral("next"), QStringLiteral("Next Track"),
                        QStringLiteral("play_pause"),
                        QStringLiteral("Play/Pause")};

    QVariantMap hints;
    hints.insert(QStringLiteral("desktop-entry"), QStringLiteral("picaudio"));

    // Provide local album art image file path if valid
    if (!artFilePath.isEmpty() && QFileInfo::exists(artFilePath)) {
      hints.insert(QStringLiteral("image-path"), artFilePath);
    }

    int timeoutMs = m_durationSec * 1000;

    msg << QStringLiteral("PicAudio");
    msg << m_lastNotificationId;
    msg << QStringLiteral("picaudio");
    msg << summary;
    msg << body;
    msg << actions;
    msg << hints;
    msg << timeoutMs;

    QDBusMessage reply = QDBusConnection::sessionBus().call(msg);
    if (reply.type() == QDBusMessage::ReplyMessage &&
        !reply.arguments().isEmpty()) {
      m_lastNotificationId = reply.arguments().at(0).toUInt();
      return;
    }
  }
#endif

  // Fallback to QSystemTrayIcon notification
  if (m_trayIcon && m_trayIcon->isVisible()) {
    m_trayIcon->showMessage(summary, body, QSystemTrayIcon::Information,
                            m_durationSec * 1000);
  }
}

#if defined(PICAUDIO_ENABLE_DBUS)
void DesktopNotificationAdapter::onActionInvoked(uint id,
                                                 const QString &actionKey) {
  Q_UNUSED(id);
  if (actionKey == QStringLiteral("next")) {
    emit nextRequested();
  } else if (actionKey == QStringLiteral("play_pause")) {
    emit togglePlayPauseRequested();
  }
}
#endif

} // namespace adapters
