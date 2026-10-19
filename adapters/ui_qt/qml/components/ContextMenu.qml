import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Menu {
    id: root

    property string menuType: "track" // "track", "header", "explorer"
    property var targetTrack: null
    property var targetTracks: []
    property string targetFilePath: ""
    property string targetTitle: ""
    property string targetArtist: ""
    property bool isPlaylistView: false
    property string currentPlaylistName: ""
    property string targetColumnKey: ""
    property string targetColumnTitle: ""
    property string targetAlbumTitle: ""
    property string targetAlbumArtist: ""
    property string targetFolderPath: ""
    property string targetPlaylistName: ""
    property bool targetPlaylistIsSmart: false
    property int targetQueueIndex: -1
    property bool stayOnTop: false
    property real currentOpacity: 1.0

    readonly property int selectedCount: targetTracks && targetTracks.length > 0 ? targetTracks.length : (targetTrack ? 1 : 0)
    readonly property bool isMultiSelect: selectedCount > 1

    function getEffectiveTracks() {
        if (targetTracks && targetTracks.length > 0) {
            return targetTracks;
        }
        if (targetTrack) {
            return [targetTrack];
        }
        return [];
    }

    function getEffectiveTrackIds() {
        var tracks = getEffectiveTracks();
        var ids = [];
        for (var i = 0; i < tracks.length; ++i) {
            if (tracks[i].id) {
                ids.push(tracks[i].id);
            }
        }
        return ids;
    }

    signal viewTrackPropertiesRequested()
    signal viewAlbumRequested(string album, string artist)
    signal editTagsRequested(var tracks)
    signal deleteTracksRequested(var tracks)
    signal deleteFilesFromDiskRequested(var tracks)
    signal newPlaylistRequested()
    signal playlistTracksChanged()
    signal openAudioFilesRequested()
    signal openAudioFolderRequested()
    signal renamePlaylistRequested(string playlistName)
    signal duplicatePlaylistRequested(string playlistName)
    signal exportPlaylistRequested(string playlistName)
    signal editSmartPlaylistRequested(string playlistName)
    signal deletePlaylistRequested(string playlistName)
    signal saveQueueAsPlaylistRequested()
    signal showAudioDiagnosticsRequested()
    signal showAudioSettingsRequested()
    signal exportArtworkRequested(string album, string artist)
    signal toggleMiniPlayerRequested()
    signal setOpacityRequested(real opacity)
    signal closeMiniPlayerRequested()
    signal toggleWindowRequested()
    signal showPreferencesRequested()

    background: Rectangle {
        implicitWidth: 230
        color: Theme.surfaceElevated
        border.color: Theme.panelBorder
        border.width: 1
        radius: Theme.cornerRadiusSmall
    }

    delegate: MenuItem {
        id: rootMenuItem
        implicitHeight: visible ? 26 : 0
        implicitWidth: 230

        contentItem: RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 12
            spacing: 8

            Text {
                text: rootMenuItem.text
                font.pixelSize: Theme.fontSizeSmall
                font.family: Theme.fontFamily
                color: rootMenuItem.enabled ? (rootMenuItem.highlighted ? Theme.textPrimary : Theme.textSecondary) : Theme.textMuted
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            Text {
                visible: rootMenuItem.subMenu !== null && rootMenuItem.subMenu !== undefined
                text: "›"
                font.pixelSize: 13
                font.bold: true
                color: rootMenuItem.highlighted ? Theme.accent : Theme.textMuted
            }
        }

        background: Rectangle {
            implicitWidth: 230
            implicitHeight: 26
            color: rootMenuItem.highlighted ? Theme.selection : "transparent"
            radius: Theme.cornerRadiusSmall
        }
    }

    component ContextMenuItem: MenuItem {
        id: cItem
        implicitHeight: visible ? 26 : 0
        implicitWidth: 230

        contentItem: RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            spacing: 6

            Text {
                visible: cItem.checkable
                text: cItem.checked ? "✓" : ""
                font.pixelSize: 11
                font.bold: true
                color: Theme.accent
                Layout.preferredWidth: cItem.checkable ? 14 : 0
            }

            Text {
                text: cItem.text
                font.pixelSize: Theme.fontSizeSmall
                font.family: Theme.fontFamily
                color: cItem.enabled ? (cItem.highlighted ? Theme.textPrimary : Theme.textSecondary) : Theme.textMuted
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            Text {
                visible: cItem.subMenu !== null && cItem.subMenu !== undefined
                text: "›"
                font.pixelSize: 13
                font.bold: true
                color: cItem.highlighted ? Theme.accent : Theme.textMuted
            }
        }

        background: Rectangle {
            implicitWidth: 230
            implicitHeight: 26
            color: cItem.highlighted ? Theme.selection : "transparent"
            radius: Theme.cornerRadiusSmall
        }
    }

    component ContextMenuSeparator: MenuSeparator {
        id: cSep
        implicitHeight: visible ? 5 : 0
        contentItem: Rectangle {
            implicitWidth: 210
            implicitHeight: 1
            color: Theme.panelBorder
            visible: cSep.visible
        }
    }

    component ContextSubMenu: Menu {
        id: subMenuRoot
        implicitWidth: 230

        delegate: MenuItem {
            id: sItem
            implicitHeight: visible ? 26 : 0
            implicitWidth: 230

            contentItem: RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 6

                Text {
                    visible: sItem.checkable
                    text: sItem.checked ? "✓" : ""
                    font.pixelSize: 11
                    font.bold: true
                    color: Theme.accent
                    Layout.preferredWidth: sItem.checkable ? 14 : 0
                }

                Text {
                    text: sItem.text
                    font.pixelSize: Theme.fontSizeSmall
                    font.family: Theme.fontFamily
                    color: sItem.enabled ? (sItem.highlighted ? Theme.textPrimary : Theme.textSecondary) : Theme.textMuted
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                Text {
                    visible: sItem.subMenu !== null && sItem.subMenu !== undefined
                    text: "›"
                    font.pixelSize: 13
                    font.bold: true
                    color: sItem.highlighted ? Theme.accent : Theme.textMuted
                }
            }

            background: Rectangle {
                implicitWidth: 230
                implicitHeight: 26
                color: sItem.highlighted ? Theme.selection : "transparent"
                radius: Theme.cornerRadiusSmall
            }
        }

        background: Rectangle {
            implicitWidth: 230
            color: Theme.surfaceElevated
            border.color: Theme.panelBorder
            border.width: 1
            radius: Theme.cornerRadiusSmall
        }
    }

    // 1. Play Now (Enter)
    ContextMenuItem {
        text: root.isMultiSelect ? ("Play Now (" + root.selectedCount + " Tracks) (Enter)") : "Play Now (Enter)"
        visible: root.menuType === "track"
        onTriggered: {
            var ids = root.getEffectiveTrackIds();
            if (ids.length > 0) {
                bridge.playSelectionNow(ids);
            }
        }
    }

    // 2. Play Next
    ContextMenuItem {
        text: root.isMultiSelect ? ("Play Next (" + root.selectedCount + " Tracks)") : "Play Next"
        visible: root.menuType === "track"
        onTriggered: {
            var ids = root.getEffectiveTrackIds();
            if (ids.length > 0) {
                bridge.playTracksNext(ids);
            }
        }
    }

    // 3. Add to Queue / Queue Last
    ContextMenuItem {
        text: root.isMultiSelect ? ("Add to Queue (" + root.selectedCount + " Tracks)") : "Add to Queue / Queue Last"
        visible: root.menuType === "track"
        onTriggered: {
            var ids = root.getEffectiveTrackIds();
            if (ids.length > 0) {
                bridge.queueTracksLast(ids);
            }
        }
    }

    // 4. Play Next in Album Shuffle
    ContextMenuItem {
        text: "Play Next in Album Shuffle"
        visible: root.menuType === "track"
        onTriggered: {
            var ids = root.getEffectiveTrackIds();
            if (ids.length > 0) {
                bridge.playNextInAlbumShuffle(ids);
            }
        }
    }

    ContextMenuSeparator {
        visible: root.menuType === "track"
    }

    // 5. Add to Playlist >
    ContextSubMenu {
        id: addToPlaylistMenu
        title: "Add to Playlist"
        visible: root.menuType === "track"

        ContextMenuItem {
            text: "New Playlist..."
            onTriggered: {
                root.newPlaylistRequested();
            }
        }

        ContextMenuSeparator {
            visible: bridge.availablePlaylists.length > 0
        }

        Instantiator {
            model: bridge.availablePlaylists
            onObjectAdded: (index, object) => addToPlaylistMenu.insertItem(index + 2, object)
            onObjectRemoved: (index, object) => addToPlaylistMenu.removeItem(object)
            delegate: ContextMenuItem {
                text: modelData
                onTriggered: {
                    var tracks = root.getEffectiveTracks();
                    bridge.addTracksToPlaylist(modelData, tracks);
                }
            }
        }
    }

    // 6. Show in File Manager / Open Containing Folder
    ContextMenuItem {
        text: "Show in File Manager"
        visible: root.menuType === "track"
        onTriggered: {
            var path = root.targetFilePath;
            if (!path && root.targetTracks && root.targetTracks.length > 0) {
                path = root.targetTracks[0].filePath || "";
            }
            if (path && path.length > 0) {
                bridge.showInFileManager(path);
            }
        }
    }

    // 7. View Technical Specs in Inspector
    ContextMenuItem {
        text: "View Technical Specs in Inspector"
        visible: root.menuType === "track"
        onTriggered: {
            root.viewTrackPropertiesRequested();
        }
    }

    // Go to Album (single selection only)
    ContextMenuItem {
        text: "Go to Album"
        visible: root.menuType === "track" && !root.isMultiSelect && root.targetTrack && root.targetTrack.album && root.targetTrack.album.length > 0
        onTriggered: {
            root.viewAlbumRequested(root.targetTrack.album, root.targetTrack.artist);
        }
    }

    ContextMenuSeparator {
        visible: root.menuType === "track"
    }

    // 8. Copy Information >
    ContextSubMenu {
        title: "Copy Information"
        visible: root.menuType === "track"

        ContextMenuItem {
            text: root.isMultiSelect ? "Copy Title & Artist (All)" : "Copy Title & Artist"
            onTriggered: {
                var tracks = root.getEffectiveTracks();
                var lines = [];
                for (var i = 0; i < tracks.length; ++i) {
                    var t = tracks[i];
                    lines.push((t.artist || "Unknown Artist") + " - " + (t.title || "Unknown Title"));
                }
                bridge.copyToClipboard(lines.join("\n"));
            }
        }

        ContextMenuItem {
            text: "Copy Full Metadata Summary"
            onTriggered: {
                var tracks = root.getEffectiveTracks();
                var summaries = [];
                for (var i = 0; i < tracks.length; ++i) {
                    var t = tracks[i];
                    var s = "Title: " + (t.title || "") + "\n" +
                            "Artist: " + (t.artist || "") + "\n" +
                            "Album: " + (t.album || "") + "\n" +
                            "Genre: " + (t.genre || "") + "\n" +
                            "Year: " + (t.year || "") + "\n" +
                            "Track: " + (t.trackNumber || "") + "\n" +
                            "Duration: " + (t.durationFormatted || "") + "\n" +
                            "Format: " + (t.codec || "") + (t.bitDepth ? (" • " + t.bitDepth + "-bit") : "") + (t.sampleRate ? (" / " + (t.sampleRate / 1000) + "kHz") : "") + "\n" +
                            "Bitrate: " + (t.bitrate ? (t.bitrate + " kbps") : "") + "\n" +
                            "File Path: " + (t.filePath || "");
                    summaries.push(s);
                }
                bridge.copyToClipboard(summaries.join("\n\n---\n\n"));
            }
        }

        ContextMenuItem {
            text: root.isMultiSelect ? "Copy Audio File Paths" : "Copy Audio File Path"
            onTriggered: {
                var tracks = root.getEffectiveTracks();
                var paths = [];
                for (var i = 0; i < tracks.length; ++i) {
                    if (tracks[i].filePath) paths.push(tracks[i].filePath);
                }
                bridge.copyToClipboard(paths.join("\n"));
            }
        }
    }

    // 9. Rating >
    ContextSubMenu {
        title: "Rating"
        visible: root.menuType === "track"

        property bool isFav: root.targetTrack && root.targetTrack.id ? bridge.isTrackFavorite(root.targetTrack.id) : false
        property int curRating: root.targetTrack && root.targetTrack.id ? bridge.getTrackRating(root.targetTrack.id) : 0

        ContextMenuItem {
            text: parent.isFav ? "❤️  Favorite (Remove)" : "🤍  Favorite (Add)"
            onTriggered: {
                var ids = root.getEffectiveTrackIds();
                bridge.toggleTracksFavorite(ids);
            }
        }

        ContextMenuSeparator {}

        ContextMenuItem {
            text: "☆☆☆☆☆  No Rating" + (parent.curRating === 0 ? "  ✓" : "")
            onTriggered: {
                var ids = root.getEffectiveTrackIds();
                bridge.setTracksRating(ids, 0);
            }
        }

        ContextMenuItem {
            text: "★☆☆☆☆  1 Star" + (parent.curRating === 1 ? "  ✓" : "")
            onTriggered: {
                var ids = root.getEffectiveTrackIds();
                bridge.setTracksRating(ids, 1);
            }
        }

        ContextMenuItem {
            text: "★★☆☆☆  2 Stars" + (parent.curRating === 2 ? "  ✓" : "")
            onTriggered: {
                var ids = root.getEffectiveTrackIds();
                bridge.setTracksRating(ids, 2);
            }
        }

        ContextMenuItem {
            text: "★★★☆☆  3 Stars" + (parent.curRating === 3 ? "  ✓" : "")
            onTriggered: {
                var ids = root.getEffectiveTrackIds();
                bridge.setTracksRating(ids, 3);
            }
        }

        ContextMenuItem {
            text: "★★★★☆  4 Stars" + (parent.curRating === 4 ? "  ✓" : "")
            onTriggered: {
                var ids = root.getEffectiveTrackIds();
                bridge.setTracksRating(ids, 4);
            }
        }

        ContextMenuItem {
            text: "★★★★★  5 Stars" + (parent.curRating === 5 ? "  ✓" : "")
            onTriggered: {
                var ids = root.getEffectiveTrackIds();
                bridge.setTracksRating(ids, 5);
            }
        }
    }

    // 10. Edit Tags & Properties... (Ctrl+T)
    ContextMenuItem {
        text: root.isMultiSelect ? ("Edit Tags & Properties... (" + root.selectedCount + " Tracks) (Ctrl+T)") : "Edit Tags & Properties... (Ctrl+T)"
        visible: root.menuType === "track"
        onTriggered: root.editTagsRequested(root.getEffectiveTracks())
    }

    ContextMenuSeparator {
        visible: root.menuType === "track"
    }

    // 11. Remove from Playlist (Only visible when viewing a playlist)
    ContextMenuItem {
        text: root.isMultiSelect ? ("Remove " + root.selectedCount + " Tracks from Playlist") : "Remove from Playlist"
        visible: root.menuType === "track" && root.isPlaylistView
        onTriggered: {
            var tracks = root.getEffectiveTracks();
            bridge.removeTracksFromPlaylist(root.currentPlaylistName, tracks);
            root.playlistTracksChanged();
        }
    }

    // 12. Remove from Library / Prune Record
    ContextMenuItem {
        text: root.isMultiSelect ? ("Remove " + root.selectedCount + " Tracks from Library") : "Remove from Library / Prune Record"
        visible: root.menuType === "track"
        onTriggered: root.deleteTracksRequested(root.getEffectiveTracks())
    }

    // 13. Delete File from Disk...
    ContextMenuItem {
        text: root.isMultiSelect ? ("Delete " + root.selectedCount + " Files from Disk...") : "Delete File from Disk..."
        visible: root.menuType === "track"
        onTriggered: root.deleteFilesFromDiskRequested(root.getEffectiveTracks())
    }

    // --- Column Header Context Menu (Issue #11) ---

    ContextMenuItem {
        text: "Sort Ascending" + (bridge.trackModel.sortColumn === root.targetColumnKey && bridge.trackModel.sortAscending ? "  ✓" : "")
        visible: root.menuType === "header" && root.targetColumnKey.length > 0
        onTriggered: {
            bridge.trackModel.sortBy(root.targetColumnKey, true);
        }
    }

    ContextMenuItem {
        text: "Sort Descending" + (bridge.trackModel.sortColumn === root.targetColumnKey && !bridge.trackModel.sortAscending ? "  ✓" : "")
        visible: root.menuType === "header" && root.targetColumnKey.length > 0
        onTriggered: {
            bridge.trackModel.sortBy(root.targetColumnKey, false);
        }
    }

    ContextMenuSeparator {
        visible: root.menuType === "header" && root.targetColumnKey.length > 0
    }

    ContextMenuItem {
        text: root.targetColumnTitle ? ("Auto-Size Column (" + root.targetColumnTitle + ")") : "Auto-Size Selected Column"
        visible: root.menuType === "header" && root.targetColumnKey.length > 0
        onTriggered: {
            bridge.autoSizeColumn(root.targetColumnKey);
        }
    }

    ContextMenuItem {
        text: "Auto-Size All Columns"
        visible: root.menuType === "header"
        onTriggered: {
            bridge.autoSizeAllColumns();
        }
    }

    ContextMenuItem {
        text: "Reset to Default Columns & Widths"
        visible: root.menuType === "header"
        onTriggered: {
            bridge.resetTableColumns();
        }
    }

    ContextMenuSeparator {
        visible: root.menuType === "header"
    }

    ContextSubMenu {
        id: columnsVisibilitySubMenu
        title: "Columns"
        visible: root.menuType === "header"

        Instantiator {
            model: bridge.tableColumnKeys
            onObjectAdded: (index, object) => columnsVisibilitySubMenu.insertItem(index, object)
            onObjectRemoved: (index, object) => columnsVisibilitySubMenu.removeItem(object)
            delegate: ContextMenuItem {
                property var colData: bridge.tableColumns[modelData]
                checkable: true
                checked: colData && colData.visible ? true : false
                text: (colData && colData.title) ? colData.title : modelData
                onTriggered: {
                    bridge.toggleColumnVisible(modelData);
                }
            }
        }
    }

    // --- Empty Viewport Area Context Menu (Issue #11) ---

    ContextMenuItem {
        text: "Select All (Ctrl+A)"
        visible: root.menuType === "viewport"
        onTriggered: bridge.trackModel.selectAll()
    }

    ContextMenuSeparator {
        visible: root.menuType === "viewport"
    }

    ContextMenuItem {
        text: "Add Audio Files... (Ctrl+O)"
        visible: root.menuType === "viewport"
        onTriggered: root.openAudioFilesRequested()
    }

    ContextMenuItem {
        text: "Add Audio Folder... (Ctrl+Shift+O)"
        visible: root.menuType === "viewport"
        onTriggered: root.openAudioFolderRequested()
    }

    ContextMenuSeparator {
        visible: root.menuType === "viewport"
    }

    ContextSubMenu {
        title: "Sort By"
        visible: root.menuType === "viewport"

        ContextMenuItem {
            text: "Title" + (bridge.trackModel.sortColumn === "title" ? (bridge.trackModel.sortAscending ? "  ▲" : "  ▼") : "")
            onTriggered: bridge.trackModel.toggleSort("title")
        }
        ContextMenuItem {
            text: "Artist" + (bridge.trackModel.sortColumn === "artist" ? (bridge.trackModel.sortAscending ? "  ▲" : "  ▼") : "")
            onTriggered: bridge.trackModel.toggleSort("artist")
        }
        ContextMenuItem {
            text: "Album" + (bridge.trackModel.sortColumn === "album" ? (bridge.trackModel.sortAscending ? "  ▲" : "  ▼") : "")
            onTriggered: bridge.trackModel.toggleSort("album")
        }
        ContextMenuItem {
            text: "Year" + (bridge.trackModel.sortColumn === "year" ? (bridge.trackModel.sortAscending ? "  ▲" : "  ▼") : "")
            onTriggered: bridge.trackModel.toggleSort("year")
        }
        ContextMenuItem {
            text: "Track Number" + (bridge.trackModel.sortColumn === "trackNumber" ? (bridge.trackModel.sortAscending ? "  ▲" : "  ▼") : "")
            onTriggered: bridge.trackModel.toggleSort("trackNumber")
        }
        ContextMenuItem {
            text: "Rating" + (bridge.trackModel.sortColumn === "rating" ? (bridge.trackModel.sortAscending ? "  ▲" : "  ▼") : "")
            onTriggered: bridge.trackModel.toggleSort("rating")
        }
        ContextMenuItem {
            text: "Date Added" + (bridge.trackModel.sortColumn === "dateAdded" ? (bridge.trackModel.sortAscending ? "  ▲" : "  ▼") : "")
            onTriggered: bridge.trackModel.toggleSort("dateAdded")
        }
    }

    ContextMenuSeparator {
        visible: root.menuType === "viewport"
    }

    ContextMenuItem {
        text: "Refresh View (F5)"
        visible: root.menuType === "viewport"
        onTriggered: {
            bridge.trackModel.loadAllTracks();
            bridge.rescanAllMonitoredFolders();
        }
    }

    // Explorer actions
    ContextMenuSeparator {
        visible: root.menuType === "explorer"
    }

    ContextMenuItem {
        text: "Rescan Library Folders"
        visible: root.menuType === "explorer"
        onTriggered: bridge.rescanAllMonitoredFolders()
    }

    ContextMenuItem {
        text: "Remove Missing Files"
        visible: root.menuType === "explorer"
        onTriggered: bridge.purgeMissingTracks()
    }

    ContextMenuItem {
        text: "Clear Library Database"
        visible: root.menuType === "explorer"
        onTriggered: bridge.clearLibrary()
    }

    ContextMenuSeparator {
        visible: root.menuType === "explorer"
    }

    ContextMenuItem {
        text: (bridge.groupAlbumsByTitle ? "Disable" : "Enable") + " Guest Artist Album Grouping"
        visible: root.menuType === "explorer"
        onTriggered: bridge.setGroupAlbumsByTitle(!bridge.groupAlbumsByTitle)
    }

    // --- Album Cards Context Menu (Issue #12) ---

    ContextMenuItem {
        text: "Play Album"
        visible: root.menuType === "album"
        onTriggered: bridge.playAlbumNow(root.targetAlbumTitle, root.targetAlbumArtist)
    }

    ContextMenuItem {
        text: "Play Album Next"
        visible: root.menuType === "album"
        onTriggered: bridge.playAlbumNext(root.targetAlbumTitle, root.targetAlbumArtist)
    }

    ContextMenuItem {
        text: "Queue Album Last"
        visible: root.menuType === "album"
        onTriggered: bridge.queueAlbumLast(root.targetAlbumTitle, root.targetAlbumArtist)
    }

    ContextMenuSeparator {
        visible: root.menuType === "album"
    }

    ContextSubMenu {
        id: addAlbumToPlaylistSubMenu
        title: "Add Album to Playlist"
        visible: root.menuType === "album"

        ContextMenuItem {
            text: "New Playlist..."
            onTriggered: {
                root.newPlaylistRequested();
            }
        }

        ContextMenuSeparator {
            visible: bridge.availablePlaylists.length > 0
        }

        Instantiator {
            model: bridge.availablePlaylists
            onObjectAdded: (index, object) => addAlbumToPlaylistSubMenu.insertItem(index + 2, object)
            onObjectRemoved: (index, object) => addAlbumToPlaylistSubMenu.removeItem(object)
            delegate: ContextMenuItem {
                text: modelData
                onTriggered: {
                    bridge.addAlbumToPlaylist(root.targetAlbumTitle, root.targetAlbumArtist, modelData);
                }
            }
        }
    }

    ContextMenuSeparator {
        visible: root.menuType === "album"
    }

    ContextMenuItem {
        text: "Show Album in File Manager"
        visible: root.menuType === "album"
        onTriggered: bridge.showAlbumInFileManager(root.targetAlbumTitle, root.targetAlbumArtist)
    }

    ContextMenuItem {
        text: "Edit Album Tags..."
        visible: root.menuType === "album"
        onTriggered: {
            var trks = bridge.getAlbumTracks(root.targetAlbumTitle, root.targetAlbumArtist);
            root.editTagsRequested(trks);
        }
    }

    ContextMenuSeparator {
        visible: root.menuType === "album"
    }

    ContextMenuItem {
        text: "Search & Fetch Album Artwork Online..."
        visible: root.menuType === "album"
        onTriggered: bridge.searchAlbumArtworkOnline(root.targetAlbumTitle, root.targetAlbumArtist)
    }

    ContextMenuItem {
        text: "Extract / Save Embedded Artwork to Disk..."
        visible: root.menuType === "album"
        onTriggered: root.exportArtworkRequested(root.targetAlbumTitle, root.targetAlbumArtist)
    }

    ContextMenuSeparator {
        visible: root.menuType === "album"
    }

    ContextMenuItem {
        text: bridge.groupAlbumsByTitle ? "Split Albums by Artist Combinations" : "Group Albums with Guest/Featuring Artists"
        visible: root.menuType === "album"
        onTriggered: bridge.setGroupAlbumsByTitle(!bridge.groupAlbumsByTitle)
    }

    // --- Left Library Explorer: Monitored Folders (Issue #12) ---

    ContextMenuItem {
        text: "Rescan Folder Now"
        visible: root.menuType === "folder"
        onTriggered: bridge.rescanMonitoredFolder(root.targetFolderPath)
    }

    ContextMenuItem {
        text: "Open Folder in File Manager"
        visible: root.menuType === "folder"
        onTriggered: bridge.showInFileManager(root.targetFolderPath)
    }

    ContextMenuSeparator {
        visible: root.menuType === "folder"
    }

    ContextMenuItem {
        text: "Remove Monitored Folder"
        visible: root.menuType === "folder"
        onTriggered: bridge.removeMonitoredFolderPath(root.targetFolderPath)
    }

    // --- Left Library Explorer: Playlists (Issue #12) ---

    ContextMenuItem {
        text: "Play"
        visible: root.menuType === "playlist"
        onTriggered: bridge.playPlaylist(root.targetPlaylistName)
    }

    ContextMenuItem {
        text: "Play Next"
        visible: root.menuType === "playlist"
        onTriggered: bridge.playPlaylistNext(root.targetPlaylistName)
    }

    ContextMenuItem {
        text: "Enqueue"
        visible: root.menuType === "playlist"
        onTriggered: bridge.queuePlaylistLast(root.targetPlaylistName)
    }

    ContextMenuSeparator {
        visible: root.menuType === "playlist"
    }

    ContextMenuItem {
        text: "Edit Rules..."
        visible: root.menuType === "playlist" && root.targetPlaylistIsSmart
        onTriggered: root.editSmartPlaylistRequested(root.targetPlaylistName)
    }

    ContextMenuItem {
        text: "Rename (F2)"
        visible: root.menuType === "playlist" && !root.targetPlaylistIsSmart
        onTriggered: root.renamePlaylistRequested(root.targetPlaylistName)
    }

    ContextMenuItem {
        text: "Duplicate"
        visible: root.menuType === "playlist"
        onTriggered: bridge.duplicatePlaylist(root.targetPlaylistName)
    }

    ContextMenuItem {
        text: root.targetPlaylistIsSmart ? "Export Playlist..." : "Export (M3U/M3U8)..."
        visible: root.menuType === "playlist"
        onTriggered: root.exportPlaylistRequested(root.targetPlaylistName)
    }

    ContextMenuSeparator {
        visible: root.menuType === "playlist"
    }

    ContextMenuItem {
        text: "Delete"
        visible: root.menuType === "playlist"
        onTriggered: root.deletePlaylistRequested(root.targetPlaylistName)
    }

    // --- Play Queue Panel Context Menu (Issue #12) ---

    ContextMenuItem {
        text: "Play Selected Item"
        visible: root.menuType === "queue" && root.targetQueueIndex >= 0
        onTriggered: bridge.playQueueTrack(root.targetQueueIndex)
    }

    ContextMenuItem {
        text: "Move Up (Alt+Up)"
        visible: root.menuType === "queue" && root.targetQueueIndex > 0
        onTriggered: bridge.moveQueueItem(root.targetQueueIndex, root.targetQueueIndex - 1)
    }

    ContextMenuItem {
        text: "Move Down (Alt+Down)"
        visible: root.menuType === "queue" && root.targetQueueIndex >= 0 && root.targetQueueIndex < bridge.queueRemainingCount - 1
        onTriggered: bridge.moveQueueItem(root.targetQueueIndex, root.targetQueueIndex + 1)
    }

    ContextMenuItem {
        text: "Remove from Queue (Delete)"
        visible: root.menuType === "queue" && root.targetQueueIndex >= 0
        onTriggered: bridge.removeFromQueue(root.targetQueueIndex)
    }

    ContextMenuSeparator {
        visible: root.menuType === "queue"
    }

    ContextMenuItem {
        text: "Clear Upcoming Queue"
        visible: root.menuType === "queue"
        onTriggered: bridge.clearQueue()
    }

    ContextMenuItem {
        text: "Shuffle Remaining Items"
        visible: root.menuType === "queue"
        onTriggered: bridge.shuffleRemainingQueue()
    }

    ContextMenuSeparator {
        visible: root.menuType === "queue"
    }

    ContextMenuItem {
        text: "Save Queue as Static Playlist..."
        visible: root.menuType === "queue"
        onTriggered: root.saveQueueAsPlaylistRequested()
    }

    // --- Bottom Transport Deck: Seek Bar (Issue #13) ---

    ContextMenuItem {
        text: "Jump to 10%"
        visible: root.menuType === "seekbar"
        onTriggered: bridge.seek(bridge.durationMs * 0.10)
    }

    ContextMenuItem {
        text: "Jump to 25%"
        visible: root.menuType === "seekbar"
        onTriggered: bridge.seek(bridge.durationMs * 0.25)
    }

    ContextMenuItem {
        text: "Jump to 50%"
        visible: root.menuType === "seekbar"
        onTriggered: bridge.seek(bridge.durationMs * 0.50)
    }

    ContextMenuItem {
        text: "Jump to 75%"
        visible: root.menuType === "seekbar"
        onTriggered: bridge.seek(bridge.durationMs * 0.75)
    }

    ContextMenuItem {
        text: "Jump to 90%"
        visible: root.menuType === "seekbar"
        onTriggered: bridge.seek(bridge.durationMs * 0.90)
    }

    ContextMenuSeparator {
        visible: root.menuType === "seekbar"
    }

    ContextMenuItem {
        text: "Copy Current Timestamp"
        visible: root.menuType === "seekbar"
        onTriggered: bridge.copyToClipboard(bridge.positionStr + " / " + bridge.durationStr)
    }

    ContextMenuSeparator {
        visible: root.menuType === "seekbar"
    }

    ContextMenuItem {
        text: "Toggle Elapsed / Remaining Time"
        visible: root.menuType === "seekbar"
        checkable: true
        checked: bridge.showRemainingTime
        onTriggered: bridge.showRemainingTime = !bridge.showRemainingTime
    }

    // --- Bottom Transport Deck: Volume Slider (Issue #13) ---

    ContextMenuItem {
        text: bridge.isMuted ? "Unmute" : "Mute"
        visible: root.menuType === "volume"
        checkable: true
        checked: bridge.isMuted
        onTriggered: bridge.toggleMute()
    }

    ContextMenuSeparator {
        visible: root.menuType === "volume"
    }

    ContextSubMenu {
        title: "Preset Volume"
        visible: root.menuType === "volume"

        ContextMenuItem {
            text: "25%"
            onTriggered: bridge.setVolume(0.25)
        }
        ContextMenuItem {
            text: "50%"
            onTriggered: bridge.setVolume(0.50)
        }
        ContextMenuItem {
            text: "75%"
            onTriggered: bridge.setVolume(0.75)
        }
        ContextMenuItem {
            text: "100%"
            onTriggered: bridge.setVolume(1.00)
        }
    }

    ContextMenuSeparator {
        visible: root.menuType === "volume"
    }

    ContextMenuItem {
        text: "Lock Volume Slider"
        visible: root.menuType === "volume"
        checkable: true
        checked: bridge.volumeLocked
        onTriggered: bridge.volumeLocked = !bridge.volumeLocked
    }

    // --- Bottom Transport Deck: Codec Badge (Issue #13) ---

    ContextMenuItem {
        text: "Audio Diagnostics... (Ctrl+J)"
        visible: root.menuType === "codec"
        onTriggered: root.showAudioDiagnosticsRequested()
    }

    ContextMenuItem {
        text: "Audio Output & Hardware Pipeline Settings..."
        visible: root.menuType === "codec"
        onTriggered: root.showAudioSettingsRequested()
    }

    // --- Mini-Player Context Menu (Issue #13) ---

    ContextMenuItem {
        text: "Stay on Top (Pin Window)"
        visible: root.menuType === "miniplayer"
        checkable: true
        checked: root.stayOnTop
        onTriggered: root.stayOnTop = !root.stayOnTop
    }

    ContextMenuItem {
        text: "Switch to Full Player Mode (Ctrl+Shift+M / F10)"
        visible: root.menuType === "miniplayer"
        onTriggered: root.toggleMiniPlayerRequested()
    }

    ContextMenuSeparator {
        visible: root.menuType === "miniplayer"
    }

    ContextSubMenu {
        title: "Window Opacity"
        visible: root.menuType === "miniplayer"

        ContextMenuItem {
            text: "100%"
            checkable: true
            checked: Math.abs(root.currentOpacity - 1.0) < 0.05
            onTriggered: root.setOpacityRequested(1.0)
        }
        ContextMenuItem {
            text: "90%"
            checkable: true
            checked: Math.abs(root.currentOpacity - 0.9) < 0.05
            onTriggered: root.setOpacityRequested(0.9)
        }
        ContextMenuItem {
            text: "75%"
            checkable: true
            checked: Math.abs(root.currentOpacity - 0.75) < 0.05
            onTriggered: root.setOpacityRequested(0.75)
        }
        ContextMenuItem {
            text: "50%"
            checkable: true
            checked: Math.abs(root.currentOpacity - 0.5) < 0.05
            onTriggered: root.setOpacityRequested(0.5)
        }
    }

    ContextMenuSeparator {
        visible: root.menuType === "miniplayer"
    }

    ContextMenuItem {
        text: "Close Mini-Player"
        visible: root.menuType === "miniplayer"
        onTriggered: root.closeMiniPlayerRequested()
    }

    // --- System Tray Icon Context Menu (Issue #13) ---

    ContextMenuItem {
        text: bridge.isPlaying ? ("Now Playing: " + bridge.currentTrackTitle + " - " + bridge.currentArtist) : "PicAudio (Stopped)"
        visible: root.menuType === "tray"
        enabled: false
    }

    ContextMenuSeparator {
        visible: root.menuType === "tray"
    }

    ContextMenuItem {
        text: bridge.isPlaying ? "Pause" : "Play"
        visible: root.menuType === "tray"
        onTriggered: bridge.togglePlayPause()
    }

    ContextMenuItem {
        text: "Stop"
        visible: root.menuType === "tray"
        onTriggered: bridge.stop()
    }

    ContextMenuItem {
        text: "Next Track"
        visible: root.menuType === "tray"
        onTriggered: bridge.nextTrack()
    }

    ContextMenuItem {
        text: "Previous Track"
        visible: root.menuType === "tray"
        onTriggered: bridge.previousTrack()
    }

    ContextMenuSeparator {
        visible: root.menuType === "tray"
    }

    ContextMenuItem {
        text: bridge.isMuted ? "Unmute" : "Mute"
        visible: root.menuType === "tray"
        onTriggered: bridge.toggleMute()
    }

    ContextSubMenu {
        title: "Volume"
        visible: root.menuType === "tray"

        ContextMenuItem { text: "0% (Mute)"; onTriggered: bridge.setVolume(0.0) }
        ContextMenuItem { text: "25%"; onTriggered: bridge.setVolume(0.25) }
        ContextMenuItem { text: "50%"; onTriggered: bridge.setVolume(0.50) }
        ContextMenuItem { text: "75%"; onTriggered: bridge.setVolume(0.75) }
        ContextMenuItem { text: "100%"; onTriggered: bridge.setVolume(1.00) }
    }

    ContextMenuSeparator {
        visible: root.menuType === "tray"
    }

    ContextMenuItem {
        text: "Show / Hide PicAudio Window"
        visible: root.menuType === "tray"
        onTriggered: root.toggleWindowRequested()
    }

    ContextMenuItem {
        text: "Preferences..."
        visible: root.menuType === "tray"
        onTriggered: root.showPreferencesRequested()
    }

    ContextMenuSeparator {
        visible: root.menuType === "tray"
    }

    ContextMenuItem {
        text: "Quit PicAudio"
        visible: root.menuType === "tray"
        onTriggered: Qt.quit()
    }
}
