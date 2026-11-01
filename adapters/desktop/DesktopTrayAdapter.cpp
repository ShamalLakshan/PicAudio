#include "DesktopTrayAdapter.hpp"

#include <QColor>
#include <QCoreApplication>
#include <QFile>
#include <QGuiApplication>
#include <QPainter>
#include <QPen>
#include <QPixmap>

namespace adapters {

DesktopTrayAdapter::DesktopTrayAdapter(QObject *parent) : QObject(parent) {
  if (QGuiApplication::instance()) {
    m_trayIcon = new QSystemTrayIcon(this);
    connect(m_trayIcon, &QSystemTrayIcon::activated, this,
            &DesktopTrayAdapter::onTrayActivated);
  }
}

DesktopTrayAdapter::~DesktopTrayAdapter() {
  if (m_trayIcon) {
    m_trayIcon->hide();
  }
}

bool DesktopTrayAdapter::initialize() {
  if (!QGuiApplication::instance() ||
      !QSystemTrayIcon::isSystemTrayAvailable()) {
    return false;
  }

  if (!m_trayIcon) {
    m_trayIcon = new QSystemTrayIcon(this);
    connect(m_trayIcon, &QSystemTrayIcon::activated, this,
            &DesktopTrayAdapter::onTrayActivated);
  }

  // Load application icon from resource or create clean geometric fallback
  if (QFile::exists(QStringLiteral(":/icons/picaudio.png"))) {
    m_baseIcon = QIcon(QStringLiteral(":/icons/picaudio.png"));
  } else if (QFile::exists(QStringLiteral(":/icons/picaudio.svg"))) {
    m_baseIcon = QIcon(QStringLiteral(":/icons/picaudio.svg"));
  } else if (QFile::exists(QStringLiteral(":/PlayerUI/icons/picaudio.png"))) {
    m_baseIcon = QIcon(QStringLiteral(":/PlayerUI/icons/picaudio.png"));
  } else {
    // Generate clean audio-themed tray icon
    QPixmap pm(32, 32);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(QColor(QStringLiteral("#ffffff")));
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(2, 2, 28, 28, 6, 6);
    p.setBrush(Qt::black);
    QPolygon triangle;
    triangle << QPoint(12, 8) << QPoint(24, 16) << QPoint(12, 24);
    p.drawPolygon(triangle);
    p.end();
    m_baseIcon = QIcon(pm);
  }

  setupMenu();
  refreshIcon();
  updateTooltip();

  m_trayIcon->show();
  return true;
}

bool DesktopTrayAdapter::isAvailable() const {
  return QSystemTrayIcon::isSystemTrayAvailable();
}

void DesktopTrayAdapter::setupMenu() {
  m_menu = new QMenu();

  m_trackInfoAction = m_menu->addAction(QStringLiteral("PicAudio"));
  m_trackInfoAction->setEnabled(false);

  m_menu->addSeparator();

  m_playPauseAction = m_menu->addAction(
      QStringLiteral("Play"), this, [this]() { emit playPauseRequested(); });
  m_nextAction = m_menu->addAction(QStringLiteral("Next Track"), this,
                                   [this]() { emit nextRequested(); });
  m_prevAction = m_menu->addAction(QStringLiteral("Previous Track"), this,
                                   [this]() { emit previousRequested(); });
  m_stopAction = m_menu->addAction(QStringLiteral("Stop"), this,
                                   [this]() { emit stopRequested(); });

  m_menu->addSeparator();

  m_volumeMenu = m_menu->addMenu(QStringLiteral("Volume"));
  m_volumeMenu->addAction(QStringLiteral("Mute / Unmute"), this,
                          [this]() { emit toggleMuteRequested(); });
  m_volumeMenu->addSeparator();
  m_volumeMenu->addAction(QStringLiteral("100%"), this,
                          [this]() { emit setVolumePresetRequested(1.0f); });
  m_volumeMenu->addAction(QStringLiteral("75%"), this,
                          [this]() { emit setVolumePresetRequested(0.75f); });
  m_volumeMenu->addAction(QStringLiteral("50%"), this,
                          [this]() { emit setVolumePresetRequested(0.5f); });
  m_volumeMenu->addAction(QStringLiteral("25%"), this,
                          [this]() { emit setVolumePresetRequested(0.25f); });

  m_menu->addSeparator();

  m_toggleWindowAction =
      m_menu->addAction(QStringLiteral("Hide Window"), this,
                        [this]() { emit toggleWindowRequested(); });

  m_menu->addSeparator();

  m_quitAction = m_menu->addAction(QStringLiteral("Quit PicAudio"), this,
                                   [this]() { emit quitRequested(); });

  m_trayIcon->setContextMenu(m_menu);
}

void DesktopTrayAdapter::updateTrackInfo(const QString &title,
                                         const QString &artist,
                                         const QString &durationStr) {
  m_currentTitle = title;
  m_currentArtist = artist;
  m_durationStr = durationStr;

  if (m_trackInfoAction) {
    if (!m_currentTitle.isEmpty()) {
      QString text =
          m_currentArtist.isEmpty()
              ? m_currentTitle
              : QStringLiteral("%1 - %2").arg(m_currentArtist, m_currentTitle);
      m_trackInfoAction->setText(text);
    } else {
      m_trackInfoAction->setText(QStringLiteral("PicAudio"));
    }
  }

  updateTooltip();
}

void DesktopTrayAdapter::updatePlaybackState(bool isPlaying) {
  m_isPlaying = isPlaying;

  if (m_playPauseAction) {
    m_playPauseAction->setText(isPlaying ? QStringLiteral("Pause")
                                         : QStringLiteral("Play"));
  }

  refreshIcon();
  updateTooltip();
}

void DesktopTrayAdapter::updatePosition(const QString &positionStr) {
  m_positionStr = positionStr;
  updateTooltip();
}

void DesktopTrayAdapter::updateWindowVisibility(bool isWindowVisible) {
  m_windowVisible = isWindowVisible;
  if (m_toggleWindowAction) {
    m_toggleWindowAction->setText(isWindowVisible
                                      ? QStringLiteral("Hide Window")
                                      : QStringLiteral("Show Window"));
  }
}

QString DesktopTrayAdapter::formattedTooltip() const {
  if (m_isPlaying || !m_currentTitle.isEmpty()) {
    QString title = m_currentTitle.isEmpty() ? QStringLiteral("Unknown Title")
                                             : m_currentTitle;
    QString artist = m_currentArtist.isEmpty()
                         ? QStringLiteral("Unknown Artist")
                         : m_currentArtist;
    QString pos =
        m_positionStr.isEmpty() ? QStringLiteral("00:00") : m_positionStr;
    QString dur =
        m_durationStr.isEmpty() ? QStringLiteral("00:00") : m_durationStr;
    return QStringLiteral("PicAudio\n%1 - %2\n[%3 / %4]")
        .arg(artist, title, pos, dur);
  }
  return QStringLiteral("PicAudio\nStopped");
}

void DesktopTrayAdapter::updateTooltip() {
  if (m_trayIcon) {
    m_trayIcon->setToolTip(formattedTooltip());
  }
}

void DesktopTrayAdapter::refreshIcon() {
  if (!m_trayIcon) {
    return;
  }

  if (!m_isPlaying) {
    m_trayIcon->setIcon(m_baseIcon);
    return;
  }

  // Draw dynamic playing indicator badge on the bottom-right corner
  QPixmap pm = m_baseIcon.pixmap(32, 32);
  if (!pm.isNull()) {
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(
        QColor(QStringLiteral("#28c840"))); // Vibrant active playback dot
    p.setPen(QPen(QColor(QStringLiteral("#000000")), 1.5));
    p.drawEllipse(QPointF(24, 24), 5, 5);
    p.end();
    m_trayIcon->setIcon(QIcon(pm));
  } else {
    m_trayIcon->setIcon(m_baseIcon);
  }
}

void DesktopTrayAdapter::onTrayActivated(
    QSystemTrayIcon::ActivationReason reason) {
  if (reason == QSystemTrayIcon::Trigger ||
      reason == QSystemTrayIcon::DoubleClick) {
    emit toggleWindowRequested();
  }
}

} // namespace adapters
