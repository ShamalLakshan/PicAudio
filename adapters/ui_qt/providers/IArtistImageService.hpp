#pragma once

#include <QByteArray>
#include <QString>
#include <functional>

namespace adapters {

/**
 * @brief Extensible interface for online artist imagery discovery services.
 * Allows Deezer, MusicBrainz, Fanart.tv, Wikidata, etc. to be implemented
 * modularly.
 */
class IArtistImageService {
public:
  virtual ~IArtistImageService() = default;

  /** @brief Returns unique provider name identifier (e.g. "Deezer"). */
  [[nodiscard]] virtual QString serviceName() const = 0;

  /** @brief Whether this provider is enabled. */
  [[nodiscard]] virtual bool isEnabled() const = 0;
  virtual void setEnabled(bool enabled) = 0;

  /**
   * @brief Completion callback type.
   * @param success True if an image was successfully found and downloaded.
   * @param imageUrl Remote source URL of the image.
   * @param imageData Raw binary image payload.
   */
  using FetchCallback = std::function<void(
      bool success, const QString &imageUrl, const QByteArray &imageData)>;

  /**
   * @brief Queries provider for artist imagery and downloads the picture data.
   * @param artistName Name of the artist to look up.
   * @param callback Completion callback.
   */
  virtual void fetchArtistImage(const QString &artistName,
                                FetchCallback callback) = 0;
};

} // namespace adapters
