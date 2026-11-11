import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../components"

Item {
    id: workspaceHostRoot

    property string activeViewId: bridge.tabModel ? bridge.tabModel.activeViewId : "tracks_default"
    property string baseRenderer: bridge.viewRegistry ? bridge.viewRegistry.getBaseRendererType(activeViewId) : "tracks"

    signal requestColumnHeaderContextMenu(string colKey, string colTitle)
    signal requestViewportContextMenu()
    signal requestTrackContextMenu(var targetTracks, var targetTrack, string targetFilePath, string targetTitle, string targetArtist, bool isPlaylistView, string currentPlaylistName)
    signal requestAlbumContextMenu(string title, string artist)
    signal requestExpandAlbum(string album, string artist)

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // In-Tab Cascading Column Browser (docked at top of main panel if visible)
        ColumnBrowserView {
            id: colBrowser
            Layout.fillWidth: true
            Layout.preferredHeight: Math.round(130 * Theme.uiScale)
            visible: bridge.layoutModel ? bridge.layoutModel.columnBrowserVisible : false
            onRequestAlbumContextMenu: (title, artist) => workspaceHostRoot.requestAlbumContextMenu(title, artist)
            onRequestSwitchToExpandedView: (title, artist) => workspaceHostRoot.requestExpandAlbum(title, artist)
        }

        // Active View Renderer
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Loader {
                id: viewLoader
                anchors.fill: parent
                sourceComponent: {
                    if (workspaceHostRoot.baseRenderer === "covers") return coversComp
                    if (workspaceHostRoot.baseRenderer === "album_and_tracks") return albumTracksComp
                    if (workspaceHostRoot.baseRenderer === "artists") return artistsComp
                    if (workspaceHostRoot.baseRenderer === "now_playing") return nowPlayingComp
                    if (workspaceHostRoot.baseRenderer === "history") return historyComp
                    if (workspaceHostRoot.baseRenderer === "jukebox") return jukeboxComp
                    return tracksComp
                }
            }

            Component {
                id: tracksComp
                TrackTableView {
                    anchors.fill: parent
                    isPlaylistView: bridge.tabModel ? bridge.tabModel.activeSourceId.indexOf("playlist://") === 0 : false
                    activePlaylistName: (bridge.tabModel && bridge.tabModel.activeSourceId.indexOf("playlist://") === 0) ? bridge.tabModel.activeSourceId.substring(11) : ""
                    onRequestColumnHeaderContextMenu: (colKey, colTitle) => workspaceHostRoot.requestColumnHeaderContextMenu(colKey, colTitle)
                    onRequestViewportContextMenu: workspaceHostRoot.requestViewportContextMenu()
                    onRequestTrackContextMenu: (targetTracks, targetTrack, targetFilePath, targetTitle, targetArtist, isPlView, plName) =>
                        workspaceHostRoot.requestTrackContextMenu(targetTracks, targetTrack, targetFilePath, targetTitle, targetArtist, isPlView, plName)
                    onRequestExpandAlbum: (album, artist) => workspaceHostRoot.requestExpandAlbum(album, artist)
                }
            }

            Component {
                id: albumTracksComp
                AlbumAndTracksView {
                    anchors.fill: parent
                }
            }

            Component {
                id: coversComp
                AlbumCoversGridView {
                    anchors.fill: parent
                    onRequestAlbumContextMenu: (title, artist) => workspaceHostRoot.requestAlbumContextMenu(title, artist)
                    onRequestTrackContextMenu: (track) => {
                        var trks = [track];
                        workspaceHostRoot.requestTrackContextMenu(trks, track, track.filePath || "", track.title || "", track.artist || "", false, "");
                    }
                }
            }

            Component {
                id: artistsComp
                ArtistsGridView {
                    anchors.fill: parent
                }
            }

            Component {
                id: nowPlayingComp
                NowPlayingView {
                    anchors.fill: parent
                }
            }

            Component {
                id: historyComp
                HistoryView {
                    anchors.fill: parent
                }
            }

            Component {
                id: jukeboxComp
                JukeboxView {
                    anchors.fill: parent
                }
            }
        }

        // A-Z Alphabetical Jump Bar (docked along bottom of main workspace if visible)
        JumpBar {
            id: jumpBar
            Layout.fillWidth: true
            visible: bridge.layoutModel ? bridge.layoutModel.jumpBarVisible : true
            onLetterSelected: function(letter) {
                bridge.filterByLetter(letter)
                if (bridge.tabModel) {
                    bridge.tabModel.updateActiveJumpBarLetter(letter)
                }
            }
        }
    }
}
