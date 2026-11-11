#pragma once

#include <core/playlist/playlist.h>
#include <QString>
#include <memory>

namespace Fooyin {
enum class ScriptSearchMode : uint8_t {
    MatchWordBeginnings = 0,
    MatchAnywhere,
};

struct ScriptSearchOptions {
    QString script;
    ScriptSearchMode mode{ScriptSearchMode::MatchWordBeginnings};
};

class TrackQueryFilter {
public:
    static constexpr auto DefaultSearchMode = 0;
    static constexpr auto DefaultSearchScript = "[%artist%] [%title%] [%album%]";

    TrackQueryFilter() = default;
    explicit TrackQueryFilter(ScriptSearchOptions) {}
    ~TrackQueryFilter() = default;

    static ScriptSearchOptions defaultSearchOptions() { return {}; }

    static bool matchesSearch(const QString& text, const QString& search, ScriptSearchMode /*mode*/, bool /*phraseMatch*/ = false) {
        return text.contains(search, Qt::CaseInsensitive);
    }

    TrackList filter(const QString& search, const TrackList& tracks) {
        if(search.isEmpty()) {
            return tracks;
        }
        TrackList res;
        for(const auto& t : tracks) {
            if(t.title().contains(search, Qt::CaseInsensitive) ||
               t.artist().contains(search, Qt::CaseInsensitive) ||
               t.album().contains(search, Qt::CaseInsensitive)) {
                res.push_back(t);
            }
        }
        return res;
    }

    PlaylistTrackList filter(const QString& search, const PlaylistTrackList& tracks) {
        if(search.isEmpty()) {
            return tracks;
        }
        PlaylistTrackList res;
        for(const auto& t : tracks) {
            if(t.track.title().contains(search, Qt::CaseInsensitive) ||
               t.track.artist().contains(search, Qt::CaseInsensitive) ||
               t.track.album().contains(search, Qt::CaseInsensitive)) {
                res.push_back(t);
            }
        }
        return res;
    }

    template <typename Context>
    PlaylistTrackList filter(const QString& search, const PlaylistTrackList& tracks, const Context&) {
        return filter(search, tracks);
    }
};
} // namespace Fooyin
