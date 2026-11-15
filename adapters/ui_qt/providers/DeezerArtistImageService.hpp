#pragma once

#include "IArtistImageService.hpp"
#include <QNetworkAccessManager>
#include <QObject>

namespace adapters {

/**
 * @brief Deezer API implementation for artist portrait discovery.
 * Queries public endpoint: https://api.deezer.com/search/artist?q={artist}
 * and downloads high-resolution artist images.
 */
class DeezerArtistImageService : public QObject, public IArtistImageService {
  Q_OBJECT

public:
  explicit DeezerArtistImageService(QNetworkAccessManager *nam = nullptr,
                                    QObject *parent = nullptr);
  ~DeezerArtistImageService() override = default;

  [[nodiscard]] QString serviceName() const override {
    return QStringLiteral("Deezer");
  }

  [[nodiscard]] bool isEnabled() const override { return m_enabled; }
  void setEnabled(bool enabled) override { m_enabled = enabled; }

  void fetchArtistImage(const QString &artistName,
                        FetchCallback callback) override;

private:
  void downloadImageBytes(const QString &imageUrl, FetchCallback callback);

  QNetworkAccessManager *m_nam{nullptr};
  bool m_ownsNam{false};
  bool m_enabled{true};
};

} // namespace adapters
