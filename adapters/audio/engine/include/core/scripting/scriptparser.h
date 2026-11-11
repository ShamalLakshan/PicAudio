#pragma once

#include <QString>
#include <core/track.h>

namespace Fooyin {

struct ParsedScript
{
    bool isValid() const { return true; }
};

class ScriptParser
{
public:
    ParsedScript parse(const QString& /*script*/) const { return {}; }

    QString evaluate(const ParsedScript& /*script*/, const Track& track) const
    {
        if(track.album().isEmpty()) {
            return {};
        }
        const QString artist = track.albumArtist().isEmpty() ? track.artist() : track.albumArtist();
        return artist + QStringLiteral(" - ") + track.album();
    }
};

} // namespace Fooyin
