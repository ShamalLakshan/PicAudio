/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fyutils_export.h"

#include <QJsonValue>
#include <QString>

#include <functional>

namespace PicAudioEngine::Utils {
using JsonArrayItemIdentity = std::function<QString(const QJsonValue&)>;

/**
 * Performs a recursive three-way merge of JSON values.
 *
 * Changes made only in @p first or @p second relative to @p base are retained. When both sides change the same value,
 * @p second wins.
 *
 * Objects are merged by key. Equal-length arrays are merged by position unless @p arrayItemIdentity is supplied.
 * Identified arrays are matched by identity and ordered according to @p second, with items added only in @p first
 * appended. If an identity is empty or duplicated, the conflicting array resolves to @p second.
 */
[[nodiscard]] PICAUDIO_UTILS_EXPORT QJsonValue mergeJsonThreeWay(const QJsonValue& base, const QJsonValue& first,
                                                          const QJsonValue& second,
                                                          const JsonArrayItemIdentity& arrayItemIdentity = {});
} // namespace PicAudioEngine::Utils
