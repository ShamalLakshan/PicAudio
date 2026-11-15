#pragma once

#include "ArtistImageManager.hpp"
#include <QPixmap>
#include <QQuickImageProvider>
#include <QString>

namespace adapters {

/**
 * @brief QQuick image provider for artist photography
 * ("image://artistart/<artist_name>"). Integrates directly with
 * ArtistImageManager for transparent disk/network resolution.
 */
class ArtistArtImageProvider : public QQuickImageProvider {
public:
  explicit ArtistArtImageProvider(ArtistImageManager *manager = nullptr);
  ~ArtistArtImageProvider() override = default;

  QPixmap requestPixmap(const QString &id, QSize *size,
                        const QSize &requestedSize) override;

  void setManager(ArtistImageManager *manager);

private:
  ArtistImageManager *m_manager{nullptr};
};

} // namespace adapters
