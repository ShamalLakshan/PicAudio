#include "DeezerArtistImageService.hpp"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>

namespace adapters {

DeezerArtistImageService::DeezerArtistImageService(QNetworkAccessManager *nam,
                                                   QObject *parent)
    : QObject(parent), m_nam(nam) {
  if (!m_nam) {
    m_nam = new QNetworkAccessManager(this);
    m_ownsNam = true;
  }
}

void DeezerArtistImageService::fetchArtistImage(const QString &artistName,
                                                FetchCallback callback) {
  QString clean = artistName.trimmed();
  if (clean.isEmpty() || !m_enabled || !m_nam) {
    if (callback) {
      callback(false, QString(), QByteArray());
    }
    return;
  }

  QUrl url(QStringLiteral("https://api.deezer.com/search/artist"));
  QUrlQuery query;
  query.addQueryItem(QStringLiteral("q"), clean);
  query.addQueryItem(QStringLiteral("limit"), QStringLiteral("1"));
  url.setQuery(query);

  QNetworkRequest request(url);
  request.setHeader(
      QNetworkRequest::UserAgentHeader,
      QStringLiteral("PicAudio/1.0 (Desktop Audio Player; Open Source)"));
  request.setTransferTimeout(7000);

  QNetworkReply *searchReply = m_nam->get(request);
  connect(searchReply, &QNetworkReply::finished, this,
          [this, searchReply, callback]() {
            searchReply->deleteLater();

            if (searchReply->error() != QNetworkReply::NoError) {
              if (callback) {
                callback(false, QString(), QByteArray());
              }
              return;
            }

            QByteArray data = searchReply->readAll();
            QJsonParseError parseErr;
            QJsonDocument doc = QJsonDocument::fromJson(data, &parseErr);
            if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
              if (callback) {
                callback(false, QString(), QByteArray());
              }
              return;
            }

            QJsonArray results =
                doc.object().value(QStringLiteral("data")).toArray();
            if (results.isEmpty()) {
              if (callback) {
                callback(false, QString(), QByteArray());
              }
              return;
            }

            QJsonObject artistObj = results.first().toObject();
            QString picUrl;
            if (artistObj.contains(QStringLiteral("picture_big"))) {
              picUrl =
                  artistObj.value(QStringLiteral("picture_big")).toString();
            } else if (artistObj.contains(QStringLiteral("picture_medium"))) {
              picUrl =
                  artistObj.value(QStringLiteral("picture_medium")).toString();
            } else if (artistObj.contains(QStringLiteral("picture_xl"))) {
              picUrl = artistObj.value(QStringLiteral("picture_xl")).toString();
            } else if (artistObj.contains(QStringLiteral("picture"))) {
              picUrl = artistObj.value(QStringLiteral("picture")).toString();
            }

            if (picUrl.isEmpty()) {
              if (callback) {
                callback(false, QString(), QByteArray());
              }
              return;
            }

            downloadImageBytes(picUrl, callback);
          });
}

void DeezerArtistImageService::downloadImageBytes(const QString &imageUrl,
                                                  FetchCallback callback) {
  QUrl url(imageUrl);
  if (!url.isValid() || !m_nam) {
    if (callback) {
      callback(false, QString(), QByteArray());
    }
    return;
  }

  QNetworkRequest request(url);
  request.setHeader(
      QNetworkRequest::UserAgentHeader,
      QStringLiteral("PicAudio/1.0 (Desktop Audio Player; Open Source)"));
  request.setTransferTimeout(8000);

  QNetworkReply *imgReply = m_nam->get(request);
  connect(imgReply, &QNetworkReply::finished, this,
          [imgReply, imageUrl, callback]() {
            imgReply->deleteLater();

            if (imgReply->error() != QNetworkReply::NoError) {
              if (callback) {
                callback(false, imageUrl, QByteArray());
              }
              return;
            }

            QByteArray imgBytes = imgReply->readAll();
            if (imgBytes.isEmpty()) {
              if (callback) {
                callback(false, imageUrl, QByteArray());
              }
              return;
            }

            if (callback) {
              callback(true, imageUrl, imgBytes);
            }
          });
}

} // namespace adapters
