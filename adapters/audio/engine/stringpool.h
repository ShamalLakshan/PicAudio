/*
 * PicAudio Engine
 * Based on the Fooyin audio engine (Copyright © Luke Taylor <luket@pm.me>).
 * Licensed under GNU General Public License version 3.
 */

#pragma once

#include "fycore_export.h"

#include <QStringList>

namespace PicAudioEngine {
class StringPoolPrivate;

/*!
 * Shared vocabulary for metadata strings.
 *
 * `StringPool` stores one canonical copy of repeated strings per metadata
 * domain and lets callers keep compact ids or packed list references instead
 * of resident `QString`/`QStringList` copies.
 *
 * The pool is intentionally append-only for the current session. This keeps
 * lookups cheap and makes the same vocabulary reusable by a higher-level owner
 * such as a library metadata store.
 *
 * Thread safety:
 * All public methods are internally synchronized. Returned ids and list refs
 * remain valid for the process lifetime.
 */
class PICAUDIO_ENGINE_EXPORT StringPool
{
public:
    using StringId = uint32_t;

    static constexpr StringId EmptyStringId = 0;

    /*!
     * Slice into the packed id storage for a multivalue field.
     */
    struct StringListRef
    {
        uint32_t offset{0};
        uint32_t size{0};

        [[nodiscard]] bool isEmpty() const
        {
            return size == 0;
        }
    };

    /*!
     * Independent vocabularies for fields that should not share ids.
     */
    enum class Domain : uint8_t
    {
        Artist = 0,
        Album,
        AlbumArtist,
        Genre,
        Composer,
        Performer,
        Codec,
        Encoding,
        ExtraTagKey,
    };

    StringPool();
    ~StringPool();

    //! Interns a value and returns the canonical string for the domain.
    [[nodiscard]] QString intern(Domain domain, const QString& value);
    //! Interns a value and returns its stable process-local id.
    [[nodiscard]] StringId internId(Domain domain, const QString& value);
    //! Packs a multivalue field into the domain's list arena.
    [[nodiscard]] StringListRef internList(Domain domain, const QStringList& values);

    //! Resolves a single id back to a string.
    [[nodiscard]] QString resolve(Domain domain, StringId id) const;
    //! Resolves a packed list into a materialized `QStringList`.
    [[nodiscard]] QStringList resolveList(Domain domain, StringListRef ref) const;
    //! Joins a packed list without exposing the caller to the backing storage.
    [[nodiscard]] QString joined(Domain domain, StringListRef ref, const QString& separator) const;

    //! Random access into a packed list.
    [[nodiscard]] QString valueAt(Domain domain, StringListRef ref, qsizetype index) const;
    //! Membership check without resolving the full list.
    [[nodiscard]] bool contains(Domain domain, StringListRef ref, const QString& value) const;

    //! Sorted snapshot of all known values in a domain.
    [[nodiscard]] QStringList values(Domain domain) const;

private:
    std::unique_ptr<StringPoolPrivate> p;
};
} // namespace PicAudioEngine
