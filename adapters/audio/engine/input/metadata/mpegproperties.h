/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include <QByteArrayView>

namespace PicAudioEngine {
class Track;

namespace Metadata {
//! Read MPEG and Xing/LAME properties from the first audio frame.
PICAUDIO_ENGINE_EXPORT void readMpegProperties(QByteArrayView data, Track& track);
} // namespace Metadata
} // namespace PicAudioEngine
