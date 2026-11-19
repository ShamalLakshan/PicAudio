#pragma once

#include "core/ports/IAudioEnginePort.hpp"
#include <QObject>
#include <QString>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

class QNetworkAccessManager;

namespace adapters {

/**
 * @brief Equalization profile for a specific headphone or IEM model.
 */
struct AutoEqProfile {
  QString id;
  QString model;
  QString source;
  QString target;
  QString formFactor;
  float preampDb{0.0f};
  std::vector<core::PeqFilterBand> filters;
};

/**
 * @brief Hybrid database providing offline search over bundled headphone/IEM
 * measurement profiles and on-demand synchronization with the AutoEq
 * repository.
 *
 * Attributions:
 * - AutoEq project by Jaakko Pasanen (MIT License)
 * - Acoustic measurements by Oratory1990, Crinacle, Rtings, Super* Review
 * - Harman Target Curves (Harman International)
 */
class AutoEqDatabase : public QObject {
  Q_OBJECT

public:
  static AutoEqDatabase &instance();

  /**
   * @brief Initializes the database by loading bundled and cached profiles.
   */
  void initialize();

  /**
   * @brief Searches database for matching headphone/IEM profiles.
   * @param query Search query text (e.g. "HD 600", "Blessing 2", "Zero").
   * @param maxResults Maximum results to return (default 30).
   * @return List of matching profiles.
   */
  [[nodiscard]] std::vector<AutoEqProfile> search(const QString &query,
                                                  int maxResults = 30) const;

  /**
   * @brief Returns total number of loaded profiles.
   */
  [[nodiscard]] size_t profileCount() const;

  /**
   * @brief Retrieves a specific profile by unique ID.
   */
  [[nodiscard]] std::optional<AutoEqProfile>
  getProfile(const QString &id) const;

  /**
   * @brief Fetches and caches a profile asynchronously from a remote URL.
   * @param url Direct raw URL to ParametricEQ.txt or GraphicEq.txt.
   * @param modelName Headphone model label.
   * @param source Measurement source label.
   * @param callback Callback delivering success status and parsed profile.
   */
  void fetchOnlineProfile(
      const QString &url, const QString &modelName, const QString &source,
      std::function<void(bool success, const AutoEqProfile &profile)> callback);

  /**
   * @brief Returns attribution notice string.
   */
  static QString attributionText();

private:
  explicit AutoEqDatabase(QObject *parent = nullptr);
  ~AutoEqDatabase() override = default;

  void loadBundledProfiles();
  void loadCachedProfiles();
  void saveProfileToDisk(const AutoEqProfile &profile);

  mutable std::mutex m_mutex;
  std::vector<AutoEqProfile> m_profiles;
  bool m_initialized{false};
  QNetworkAccessManager *m_networkManager{nullptr};
};

} // namespace adapters
