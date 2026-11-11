/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include <core/stringpool.h>

namespace PicAudioEngine {
/*!
 * Explicit owner for shared track metadata vocabularies.
 *
 * A library can keep one `TrackMetadataStore` and let all resident `Track`
 * instances intern repeated strings through it. Standalone tracks may also own
 * their own store when no library is involved.
 */
class PICAUDIO_ENGINE_EXPORT TrackMetadataStore
{
public:
    [[nodiscard]] StringPool& stringPool()
    {
        return m_stringPool;
    }

    [[nodiscard]] const StringPool& stringPool() const
    {
        return m_stringPool;
    }

    [[nodiscard]] QStringList values(StringPool::Domain domain) const
    {
        return m_stringPool.values(domain);
    }

private:
    StringPool m_stringPool;
};
} // namespace PicAudioEngine
