/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fyutils_export.h"

#include <QByteArray>
#include <QString>

#include <chrono>
#include <optional>

class QFontMetrics;
class QJsonValue;
class QKeySequence;

namespace PicAudioEngine::Utils {
constexpr auto PreferredFallbackEncodingSetting = "Encoding/PreferredFallbackEncoding";

struct DetectEncodingOptions
{
    QByteArray preferredFallbackEncoding;
};

PICAUDIO_UTILS_EXPORT QString readMultiLineString(const QJsonValue& value);
PICAUDIO_UTILS_EXPORT QString elideTextWithBreaks(const QString& text, const QFontMetrics& fontMetrics, int maxWidth,
                                           Qt::TextElideMode mode);
PICAUDIO_UTILS_EXPORT QString capitalise(const QString& str);
PICAUDIO_UTILS_EXPORT QByteArray detectEncoding(const QByteArray& content, const DetectEncodingOptions& options = {});
PICAUDIO_UTILS_EXPORT std::optional<QString> decodeText(const QByteArray& content, const QByteArray& encoding);
PICAUDIO_UTILS_EXPORT QString decodeTextWithDetection(const QByteArray& content, const DetectEncodingOptions& options = {});
PICAUDIO_UTILS_EXPORT QString foldForSearch(QStringView text);
PICAUDIO_UTILS_EXPORT bool containsWord(QStringView text, QStringView word);

PICAUDIO_UTILS_EXPORT QString msToString(std::chrono::milliseconds ms, bool includeMs);
PICAUDIO_UTILS_EXPORT QString msToString(uint64_t ms);
PICAUDIO_UTILS_EXPORT QString formatFileSize(uint64_t bytes, bool includeBytes = false);
PICAUDIO_UTILS_EXPORT QString addLeadingZero(int number, int leadingCount);
PICAUDIO_UTILS_EXPORT QString appendShortcut(const QString& str, const QKeySequence& shortcut);

PICAUDIO_UTILS_EXPORT int levenshteinDistance(const QString& first, const QString& second, Qt::CaseSensitivity cs);
PICAUDIO_UTILS_EXPORT int similarityRatio(const QString& first, const QString& second, Qt::CaseSensitivity cs);
} // namespace PicAudioEngine::Utils
