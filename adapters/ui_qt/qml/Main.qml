import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Qt.labs.platform as Platform
import "components"
import "dialogs"
import "views"
import "menubar"
import "toolbar"
import "navigation"
import "inspector"
import "player"
import "statusbar"

ApplicationWindow {
    id: root
    width: 1400
    height: 840
    minimumWidth: 1024
    minimumHeight: 640
    visible: true
    title: "PicAudio Music Player"
    color: Theme.background
    flags: Qt.Window | (stayOnTop ? Qt.WindowStaysOnTopHint : 0)

    // Panel visibility and navigation state
    property bool stayOnTop: false
    property bool showLeftPanel: bridge.showLeftPanel
    property bool showRightPanel: bridge.showRightPanel
    property bool showColumnBrowser: bridge.showColumnBrowser
    property bool showStatusBar: true
    property bool showEqualizer: false
    property bool isMiniPlayer: false
    property real normalWidth: 1400
    property real normalHeight: 840
    property int activeNavSection: bridge.activeNavSection // 0: All Tracks, 1: Albums, 2: Artists, 3: Genres, 4: Now Playing, 5: Playlist
    property string activeLetterFilter: "All"
    property string selectedGenreFilter: "All"
    property string selectedArtistFilter: "All"
    property int rightPanelTab: 0 // 0: Properties, 1: Queue
    property string activePlaylistName: bridge.activePlaylistName

    onShowLeftPanelChanged: bridge.showLeftPanel = showLeftPanel
    onShowRightPanelChanged: bridge.showRightPanel = showRightPanel
    onShowColumnBrowserChanged: bridge.showColumnBrowser = showColumnBrowser
    onActiveNavSectionChanged: bridge.activeNavSection = activeNavSection
    onActivePlaylistNameChanged: bridge.activePlaylistName = activePlaylistName

    onShowEqualizerChanged: {
        if (showEqualizer && !isMiniPlayer) {
            if (!equalizerOverlay.visible) {
                equalizerOverlay.open();
            }
        } else {
            if (equalizerOverlay.visible) {
                equalizerOverlay.close();
            }
        }
    }

    onIsMiniPlayerChanged: {
        if (isMiniPlayer && showEqualizer) {
            showEqualizer = false;
        }
    }

    Component.onCompleted: {
        var geom = bridge.loadWindowGeometry(root.width, root.height);
        if (geom && geom.valid) {
            root.x = geom.x;
            root.y = geom.y;
            root.width = geom.width;
            root.height = geom.height;
            if (geom.maximized) {
                root.visibility = Window.Maximized;
            }
        }
        root.showLeftPanel = bridge.showLeftPanel;
        root.showRightPanel = bridge.showRightPanel;
        root.showColumnBrowser = bridge.showColumnBrowser;
        root.activeNavSection = bridge.activeNavSection;
        root.activePlaylistName = bridge.activePlaylistName;
        if (root.activePlaylistName && root.activePlaylistName.length > 0) {
            bridge.openPlaylist(root.activePlaylistName);
        }
    }

    Connections {
        target: bridge
        function onFiltersReset() {
            root.activeLetterFilter = "All";
            root.selectedGenreFilter = "All";
            root.selectedArtistFilter = "All";
            topToolbar.clearSearch();
        }
    }

    onClosing: (close) => {
        if (!isMiniPlayer && visibility !== Window.Minimized) {
            var isMax = (visibility === Window.Maximized);
            bridge.saveWindowGeometry(root.x, root.y, root.width, root.height, isMax);
        }
        bridge.showLeftPanel = root.showLeftPanel;
        bridge.showRightPanel = root.showRightPanel;
        bridge.showColumnBrowser = root.showColumnBrowser;
        bridge.activeNavSection = root.activeNavSection;
        bridge.activePlaylistName = root.activePlaylistName;
        bridge.savePlaybackSession();

        if (typeof desktopIntegration !== "undefined" && !desktopIntegration.handleWindowCloseRequest()) {
            close.accepted = false;
        }
    }

    onVisibilityChanged: {
        if (root.visibility === Window.Minimized && typeof desktopIntegration !== "undefined") {
            desktopIntegration.handleWindowMinimizeRequest();
        }
    }

    // Focus isolation state
    readonly property bool isAnyDialogOpen: preferencesDialog.visible || tagEditorDialog.visible || openUrlDialog.visible || shortcutsDialog.visible || diagnosticsDialog.visible || channelMatrixDialog.visible || aboutDialog.visible || sleepTimerDialog.visible || newPlaylistDialog.visible || renamePlaylistDialog.visible || deleteConfirmDialog.visible || libraryHealthDialog.visible || smartPlaylistRuleDialog.visible || (equalizerOverlay && equalizerOverlay.visible)
    readonly property bool isTextInputFocused: (root.activeFocusItem !== null) && ((root.activeFocusItem.echoMode !== undefined) || (root.activeFocusItem.cursorPosition !== undefined) || (root.activeFocusItem.text !== undefined && root.activeFocusItem.readOnly !== undefined && !root.activeFocusItem.readOnly))
    readonly property bool isInputActive: topToolbar.isSearchActiveFocus || isAnyDialogOpen || isTextInputFocused

    function playSelectedNow() {
        var selIds = bridge.trackModel.getSelectedTrackIds();
        if (selIds.length > 0) {
            bridge.playSelectionNow(selIds);
        } else if (bridge.trackModel.count > 0) {
            bridge.playTrackAtIndex(0);
        }
    }

    function switchToExpandedView(albumTitle, albumArtist) {
        if (albumTitle && albumTitle.length > 0) {
            bridge.openAlbumDetails(albumTitle, albumArtist ? albumArtist : "");
        } else if (bridge.selectedAlbumTitle.length === 0) {
            if (bridge.currentAlbum.length > 0) {
                bridge.openAlbumDetails(bridge.currentAlbum, bridge.currentArtist);
            } else if (bridge.albumModel.count > 0) {
                var firstAlb = bridge.albumModel.getAlbumAt(0);
                if (firstAlb && firstAlb.title) {
                    bridge.openAlbumDetails(firstAlb.title, firstAlb.artist);
                }
            }
        }
        if (bridge.tabModel) {
            bridge.tabModel.updateActiveView("album_and_tracks_default");
        }
    }

    function switchToNowPlaying() {
        if (bridge.tabModel) {
            bridge.tabModel.updateActiveView("now_playing_default");
        }
    }

    function toggleMiniPlayer() {
        if (!isMiniPlayer) {
            normalWidth = root.width;
            normalHeight = root.height;
            root.minimumWidth = 380;
            root.minimumHeight = 120;
            root.width = 440;
            root.height = 140;
            isMiniPlayer = true;
        } else {
            root.minimumWidth = 1024;
            root.minimumHeight = 640;
            root.width = Math.max(1024, normalWidth);
            root.height = Math.max(640, normalHeight);
            isMiniPlayer = false;
        }
    }

    function toggleFullscreen() {
        if (root.visibility === Window.FullScreen) {
            root.visibility = Window.Windowed;
        } else {
            root.visibility = Window.FullScreen;
        }
    }

    function zoomIn() {
        Theme.uiScale = Math.min(1.5, Math.round((Theme.uiScale + 0.1) * 10) / 10);
    }

    function zoomOut() {
        Theme.uiScale = Math.max(0.7, Math.round((Theme.uiScale - 0.1) * 10) / 10);
    }

    function zoomReset() {
        Theme.uiScale = 1.0;
    }

    function formatTimestamp(ms) {
        if (ms < 0) return "--:--";
        var totalSec = Math.floor(ms / 1000);
        var mins = Math.floor(totalSec / 60);
        var secs = totalSec % 60;
        return mins + ":" + (secs < 10 ? "0" : "") + secs;
    }

    PreferencesDialog {
        id: preferencesDialog
        onRequestLibraryHealth: {
            preferencesDialog.close();
            libraryHealthDialog.open();
        }
    }

    TagEditorDialog {
        id: tagEditorDialog
    }

    LibraryHealthDialog {
        id: libraryHealthDialog
    }

    SmartPlaylistRuleDialog {
        id: smartPlaylistRuleDialog
    }

    OpenUrlDialog {
        id: openUrlDialog
    }

    ShortcutsDialog {
        id: shortcutsDialog
    }

    DiagnosticsDialog {
        id: diagnosticsDialog
    }

    ChannelMatrixDialog {
        id: channelMatrixDialog
    }

    ArrangePanelsDialog {
        id: arrangePanelsDialogInstance
    }

    ViewConfigDialog {
        id: viewConfigDialogInstance
    }

    ConfirmDropDialog {
        id: confirmDropDialogInstance
    }

    FloatingWindow {
        id: floatingWindowInstance
    }

    AboutDialog {
        id: aboutDialog
    }

    SleepTimerDialog {
        id: sleepTimerDialog
    }

    NewPlaylistDialog {
        id: newPlaylistDialog
    }

    FileDialog {
        id: importPlaylistDialog
        title: "Import Playlist File"
        nameFilters: ["Playlist files (*.m3u *.m3u8 *.pls *.xspf)", "All files (*)"]
        onAccepted: bridge.importPlaylist(selectedFile.toString())
    }

    FileDialog {
        id: openAudioFileDialog
        title: "Open Audio File"
        nameFilters: ["Audio files (*.mp3 *.flac *.wav *.ogg *.opus *.m4a *.aac *.alac *.aiff *.ape *.wv *.dsf *.dff)", "All files (*)"]
        onAccepted: bridge.openAudioFile(selectedFile.toString())
    }

    FolderDialog {
        id: openMusicFolderDialog
        title: "Open Music Folder"
        onAccepted: bridge.openFolder(selectedFolder.toString(), false)
    }

    FileDialog {
        id: openCueSheetDialog
        title: "Open CUE Sheet"
        nameFilters: ["CUE Sheet files (*.cue)", "All files (*)"]
        onAccepted: bridge.openCueSheet(selectedFile.toString())
    }

    FileDialog {
        id: exportViewDialog
        title: "Export Active View / Playlist"
        fileMode: FileDialog.SaveFile
        nameFilters: ["M3U8 Playlist (*.m3u8)", "CSV Spreadsheet (*.csv)", "JSON Data (*.json)"]
        onAccepted: {
            var path = selectedFile.toString();
            var lower = path.toLowerCase();
            var fmt = "m3u8";
            if (lower.indexOf(".csv") !== -1) {
                fmt = "csv";
            } else if (lower.indexOf(".json") !== -1) {
                fmt = "json";
            }
            bridge.exportActiveView(path, fmt);
        }
    }

    FileDialog {
        id: exportArtworkDialog
        title: "Export Album Artwork"
        fileMode: FileDialog.SaveFile
        nameFilters: ["Image files (*.jpg *.png)", "All files (*)"]
        property string albumTitle: ""
        property string albumArtist: ""
        onAccepted: {
            bridge.exportAlbumArtwork(albumTitle, albumArtist, selectedFile.toString());
        }
    }

    FileDialog {
        id: exportPlaylistDialog
        title: "Export Playlist"
        fileMode: FileDialog.SaveFile
        nameFilters: ["M3U8 Playlist (*.m3u8)", "M3U Playlist (*.m3u)", "All files (*)"]
        property string playlistName: ""
        onAccepted: {
            bridge.exportPlaylistFile(playlistName, selectedFile.toString());
        }
    }

    RenamePlaylistDialog {
        id: renamePlaylistDialog
        onPlaylistRenamed: (oldName, newName) => {
            if (activePlaylistName === oldName) {
                activePlaylistName = newName;
            }
        }
    }

    DeleteConfirmDialog {
        id: deleteConfirmDialog
        onPlaylistDeleted: (plName) => {
            if (activePlaylistName === plName) {
                activePlaylistName = "";
            }
        }
    }

    ContextMenu {
        id: trackContextMenu
        menuType: "track"
        onViewTrackPropertiesRequested: {
            showRightPanel = true;
            rightPanelTab = 0;
        }
        onViewAlbumRequested: (album, artist) => {
            switchToExpandedView(album, artist);
        }
        onEditTagsRequested: (tracks) => {
            if (tracks && tracks.length > 1) {
                tagEditorDialog.targetTracks = tracks;
                tagEditorDialog.targetTrack = tracks[0];
            } else if (tracks && tracks.length === 1) {
                tagEditorDialog.targetTracks = [];
                tagEditorDialog.targetTrack = tracks[0];
            } else if (trackContextMenu.targetTrack) {
                tagEditorDialog.targetTracks = [];
                tagEditorDialog.targetTrack = trackContextMenu.targetTrack;
            }
            tagEditorDialog.open();
        }
        onDeleteTracksRequested: (tracks) => {
            var ids = [];
            for (var i = 0; i < tracks.length; ++i) {
                if (tracks[i].id) ids.push(tracks[i].id);
            }
            if (ids.length > 0) {
                bridge.deleteSelectedTracks(ids);
            }
        }
        onDeleteFilesFromDiskRequested: (tracks) => {
            deleteConfirmDialog.tracksToDelete = tracks;
            deleteConfirmDialog.open();
        }
        onNewPlaylistRequested: {
            newPlaylistDialog.isSmart = false;
            newPlaylistDialog.open();
        }
        onPlaylistTracksChanged: {
            if (activeNavSection === 5 && activePlaylistName.length > 0) {
                bridge.openPlaylist(activePlaylistName);
            }
        }
    }

    ContextMenu {
        id: explorerContextMenu
        menuType: "explorer"
    }

    ContextMenu {
        id: columnHeaderContextMenu
        menuType: "header"
    }

    ContextMenu {
        id: viewportContextMenu
        menuType: "viewport"
        onOpenAudioFilesRequested: openAudioFileDialog.open()
        onOpenAudioFolderRequested: openMusicFolderDialog.open()
    }

    ContextMenu {
        id: albumContextMenu
        menuType: "album"
        onEditTagsRequested: (tracks) => {
            if (tracks && tracks.length > 0) {
                tagEditorDialog.targetTracks = tracks;
                tagEditorDialog.targetTrack = tracks[0];
                tagEditorDialog.open();
            }
        }
        onExportArtworkRequested: (album, artist) => {
            exportArtworkDialog.albumTitle = album;
            exportArtworkDialog.albumArtist = artist;
            exportArtworkDialog.open();
        }
        onNewPlaylistRequested: {
            newPlaylistDialog.isSmart = false;
            newPlaylistDialog.isEdit = false;
            newPlaylistDialog.open();
        }
    }

    ContextMenu {
        id: folderContextMenu
        menuType: "folder"
    }

    ContextMenu {
        id: playlistContextMenu
        menuType: "playlist"
        onRenamePlaylistRequested: (plName) => {
            renamePlaylistDialog.oldName = plName;
            renamePlaylistDialog.open();
        }
        onExportPlaylistRequested: (plName) => {
            exportPlaylistDialog.playlistName = plName;
            exportPlaylistDialog.open();
        }
        onEditSmartPlaylistRequested: (plName) => {
            smartPlaylistRuleDialog.playlistName = plName;
            smartPlaylistRuleDialog.open();
        }
        onDeletePlaylistRequested: (plName) => {
            deleteConfirmDialog.confirmDeletePlaylist(plName);
        }
    }

    ContextMenu {
        id: seekBarContextMenu
        menuType: "seekbar"
    }

    ContextMenu {
        id: volumeContextMenu
        menuType: "volume"
    }

    ContextMenu {
        id: codecContextMenu
        menuType: "codec"
        onShowAudioDiagnosticsRequested: diagnosticsDialog.open()
        onShowAudioSettingsRequested: {
            preferencesDialog.openTab(1);
            preferencesDialog.open();
        }
    }

    Connections {
        target: bridge
        function onOpenChannelMatrixRequested() {
            channelMatrixDialog.open();
        }
    }

    ContextMenu {
        id: miniPlayerContextMenu
        menuType: "miniplayer"
        stayOnTop: root.stayOnTop
        currentOpacity: root.opacity
        onStayOnTopChanged: {
            root.stayOnTop = miniPlayerContextMenu.stayOnTop;
        }
        onToggleMiniPlayerRequested: toggleMiniPlayer()
        onSetOpacityRequested: (op) => {
            root.opacity = op;
        }
        onCloseMiniPlayerRequested: toggleMiniPlayer()
    }

    ContextMenu {
        id: trayContextMenu
        menuType: "tray"
        onToggleWindowRequested: {
            if (root.visible) {
                root.hide();
            } else {
                root.show();
                root.raise();
                root.requestActivate();
            }
        }
        onShowPreferencesRequested: {
            root.show();
            root.raise();
            root.requestActivate();
            preferencesDialog.open();
        }
    }

    Platform.SystemTrayIcon {
        id: systemTrayIcon
        visible: true
        tooltip: "PicAudio Music Player"
        menu: Platform.Menu {
            Platform.MenuItem {
                text: bridge.isPlaying ? (bridge.currentTrackTitle + " - " + bridge.currentArtist) : "PicAudio (Stopped)"
                enabled: false
            }
            Platform.MenuSeparator {}
            Platform.MenuItem {
                text: bridge.isPlaying ? "Pause" : "Play"
                onTriggered: bridge.togglePlayPause()
            }
            Platform.MenuItem {
                text: "Stop"
                onTriggered: bridge.stop()
            }
            Platform.MenuItem {
                text: "Next Track"
                onTriggered: bridge.nextTrack()
            }
            Platform.MenuItem {
                text: "Previous Track"
                onTriggered: bridge.previousTrack()
            }
            Platform.MenuSeparator {}
            Platform.MenuItem {
                text: bridge.isMuted ? "Unmute" : "Mute"
                onTriggered: bridge.toggleMute()
            }
            Platform.MenuSeparator {}
            Platform.MenuItem {
                text: root.visible ? "Hide Window" : "Show Window"
                onTriggered: {
                    if (root.visible) {
                        root.hide();
                    } else {
                        root.show();
                        root.raise();
                        root.requestActivate();
                    }
                }
            }
            Platform.MenuItem {
                text: "Preferences..."
                onTriggered: {
                    root.show();
                    root.raise();
                    root.requestActivate();
                    preferencesDialog.open();
                }
            }
            Platform.MenuSeparator {}
            Platform.MenuItem {
                text: "Quit PicAudio"
                onTriggered: Qt.quit()
            }
        }
        onActivated: (reason) => {
            if (reason === Platform.SystemTrayIcon.Trigger || reason === Platform.SystemTrayIcon.DoubleClick) {
                if (root.visible) {
                    root.hide();
                } else {
                    root.show();
                    root.raise();
                    root.requestActivate();
                }
            }
        }
    }

    // Keyboard shortcuts - Playback Transport
    Shortcut { sequence: "Space"; enabled: !isInputActive; onActivated: bridge.togglePlayPause() }
    Shortcut { sequence: "Shift+Space"; enabled: !isInputActive; onActivated: bridge.toggleStopAfterCurrentTrack() }
    Shortcut { sequence: "Ctrl+Right"; onActivated: bridge.nextTrack() }
    Shortcut { sequence: "Ctrl+Left"; onActivated: bridge.previousTrack() }
    Shortcut { sequence: "Ctrl+Up"; onActivated: bridge.setVolume(bridge.volume + 0.05) }
    Shortcut { sequence: "Ctrl+Down"; onActivated: bridge.setVolume(bridge.volume - 0.05) }
    Shortcut { sequence: "Ctrl+M"; onActivated: bridge.toggleMute() }
    Shortcut { sequence: "Ctrl+S"; onActivated: bridge.cycleShuffleMode() }
    Shortcut { sequence: "Ctrl+R"; onActivated: bridge.cycleRepeatMode() }
    Shortcut { sequence: "Ctrl+."; onActivated: bridge.stop() }
    Shortcut { sequence: "Left"; enabled: !isInputActive; onActivated: bridge.seek(bridge.positionMs - 5000) }
    Shortcut { sequence: "Right"; enabled: !isInputActive; onActivated: bridge.seek(bridge.positionMs + 5000) }
    Shortcut { sequence: "Shift+Left"; enabled: !isInputActive; onActivated: bridge.seek(bridge.positionMs - 30000) }
    Shortcut { sequence: "Shift+Right"; enabled: !isInputActive; onActivated: bridge.seek(bridge.positionMs + 30000) }
    Shortcut { sequence: "Home"; enabled: !isInputActive; onActivated: bridge.seek(0) }
    Shortcut { sequence: "End"; enabled: !isInputActive; onActivated: bridge.seek(bridge.durationMs) }
    Shortcut { sequence: "["; enabled: !isInputActive; onActivated: bridge.setLoopPointA() }
    Shortcut { sequence: "]"; enabled: !isInputActive; onActivated: bridge.setLoopPointB() }
    Shortcut { sequence: "\\"; enabled: !isInputActive; onActivated: bridge.clearLoop() }

    // Media Keys
    Shortcut { sequence: "MediaPlay"; onActivated: bridge.togglePlayPause() }
    Shortcut { sequence: "MediaPause"; onActivated: bridge.togglePlayPause() }
    Shortcut { sequence: "MediaTogglePlayPause"; onActivated: bridge.togglePlayPause() }
    Shortcut { sequence: "MediaStop"; onActivated: bridge.stop() }
    Shortcut { sequence: "MediaNext"; onActivated: bridge.nextTrack() }
    Shortcut { sequence: "MediaPrevious"; onActivated: bridge.previousTrack() }
    Shortcut { sequence: "AudioRaiseVolume"; onActivated: bridge.setVolume(bridge.volume + 0.05) }
    Shortcut { sequence: "AudioLowerVolume"; onActivated: bridge.setVolume(bridge.volume - 0.05) }
    Shortcut { sequence: "AudioMute"; onActivated: bridge.toggleMute() }

    // Keyboard shortcuts - File & Library Operations
    Shortcut { sequence: "Ctrl+O"; onActivated: openAudioFileDialog.open() }
    Shortcut { sequence: "Ctrl+Shift+O"; onActivated: openMusicFolderDialog.open() }
    Shortcut { sequence: "Ctrl+U"; onActivated: openUrlDialog.open() }
    Shortcut { sequence: "Ctrl+N"; onActivated: { newPlaylistDialog.isSmart = false; newPlaylistDialog.open(); } }
    Shortcut { sequence: "Ctrl+Shift+N"; onActivated: newPlaylistDialog.openSmart() }
    Shortcut { sequence: "F5"; onActivated: bridge.rescanAllMonitoredFolders() }
    Shortcut { sequence: "Ctrl+F5"; onActivated: bridge.incrementalQuickScan() }
    Shortcut { sequence: "Ctrl+E"; onActivated: exportViewDialog.open() }
    Shortcut { sequence: "Ctrl+,"; onActivated: preferencesDialog.open() }
    Shortcut { sequence: "Ctrl+W"; onActivated: root.hide() }
    Shortcut { sequence: "Ctrl+Q"; onActivated: Qt.quit() }

    // Keyboard shortcuts - Edit & Selection Operations
    Shortcut { sequence: "Ctrl+Z"; enabled: !isInputActive; onActivated: bridge.undo() }
    Shortcut { sequence: "Ctrl+Y"; enabled: !isInputActive; onActivated: bridge.redo() }
    Shortcut { sequence: "Ctrl+Shift+Z"; enabled: !isInputActive; onActivated: bridge.redo() }
    Shortcut { sequence: "Ctrl+A"; enabled: !isInputActive; onActivated: bridge.trackModel.selectAll() }
    Shortcut { sequence: "Ctrl+Shift+A"; enabled: !isInputActive; onActivated: bridge.trackModel.invertSelection() }
    Shortcut { sequence: "Ctrl+F"; onActivated: topToolbar.focusSearch() }
    Shortcut { sequence: "Shift+Escape"; onActivated: topToolbar.clearSearch() }
    Shortcut {
        sequence: "Ctrl+T"
        enabled: !isInputActive
        onActivated: {
            var sel = bridge.trackModel.getSelectedTracks();
            if (sel.length > 0) {
                tagEditorDialog.targetTracks = sel.length > 1 ? sel : [];
                tagEditorDialog.targetTrack = sel[0];
                tagEditorDialog.open();
            }
        }
    }
    Shortcut {
        sequence: "Return"
        enabled: !isInputActive
        onActivated: playSelectedNow()
    }
    Shortcut {
        sequence: "Enter"
        enabled: !isInputActive
        onActivated: playSelectedNow()
    }
    Shortcut {
        sequence: "Delete"
        enabled: !isInputActive
        onActivated: {
            var ids = bridge.trackModel.getSelectedTrackIds();
            if (ids.length > 0) {
                bridge.deleteSelectedTracks(ids);
            }
        }
    }
    Shortcut {
        sequence: "Escape"
        onActivated: {
            if (showEqualizer) {
                showEqualizer = false;
            } else if (bridge.trackModel.selectedCount > 0) {
                bridge.trackModel.clearSelection();
            } else if (topToolbar.searchText.length > 0) {
                topToolbar.clearSearch();
            }
        }
    }

    // Keyboard shortcuts - View & Layout Navigation
    Shortcut { sequence: "Ctrl+1"; onActivated: showLeftPanel = !showLeftPanel }
    Shortcut { sequence: "Ctrl+2"; onActivated: showColumnBrowser = !showColumnBrowser }
    Shortcut {
        sequence: "Ctrl+3"
        onActivated: {
            if (!showRightPanel || rightPanelTab !== 0) {
                showRightPanel = true;
                rightPanelTab = 0;
            } else {
                showRightPanel = false;
            }
        }
    }
    Shortcut {
        sequence: "Ctrl+4"
        onActivated: {
            if (!showRightPanel || rightPanelTab !== 1) {
                showRightPanel = true;
                rightPanelTab = 1;
            } else {
                showRightPanel = false;
            }
        }
    }
    Shortcut { sequence: "Ctrl+5"; onActivated: showEqualizer = !showEqualizer }
    Shortcut { sequence: "Alt+1"; onActivated: { if (bridge.tabModel) bridge.tabModel.updateActiveView("tracks_default") } }
    Shortcut { sequence: "Alt+2"; onActivated: { if (bridge.tabModel) bridge.tabModel.updateActiveView("covers_default") } }
    Shortcut { sequence: "Alt+3"; onActivated: { if (bridge.tabModel) bridge.tabModel.updateActiveView("album_and_tracks_default") } }
    Shortcut { sequence: "Alt+4"; onActivated: { if (bridge.tabModel) bridge.tabModel.updateActiveView("artists_default") } }
    Shortcut { sequence: "Alt+5"; onActivated: { if (bridge.tabModel) bridge.tabModel.updateActiveView("now_playing_default") } }
    Shortcut { sequence: "Alt+6"; onActivated: { if (bridge.tabModel) bridge.tabModel.updateActiveView("jukebox_default") } }
    Shortcut { sequence: "Alt+7"; onActivated: { if (bridge.tabModel) bridge.tabModel.updateActiveView("history_default") } }
    Shortcut { sequence: "Ctrl+Shift+M"; onActivated: toggleMiniPlayer() }
    Shortcut { sequence: "F10"; onActivated: toggleMiniPlayer() }
    Shortcut { sequence: "F11"; onActivated: toggleFullscreen() }
    Shortcut { sequence: "Ctrl+="; onActivated: zoomIn() }
    Shortcut { sequence: "Ctrl++"; onActivated: zoomIn() }
    Shortcut { sequence: "Ctrl+-"; onActivated: zoomOut() }
    Shortcut { sequence: "Ctrl+0"; onActivated: zoomReset() }
    Shortcut { sequence: "Ctrl+Shift+E"; onActivated: showEqualizer = !showEqualizer }
    Shortcut { sequence: "F1"; onActivated: Qt.openUrlExternally("https://github.com/ShamalLakshan/PicAudio") }
    Shortcut { sequence: "Ctrl+/"; onActivated: shortcutsDialog.open() }
    Shortcut { sequence: "Ctrl+?"; onActivated: shortcutsDialog.open() }

    FolderDialog {
        id: folderDialog
        title: "Select Directory to Scan for Music"
        currentFolder: "file:///home/shamal/Music"
        onAccepted: {
            bridge.scanDirectory(selectedFolder.toString())
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        visible: !isMiniPlayer

        // Menu bar and status
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 28
            color: Theme.surface
            clip: true

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: 4

                // Brand logo / title
                RowLayout {
                    spacing: 6
                    Layout.alignment: Qt.AlignVCenter

                    Image {
                        source: "qrc:/icons/picaudio.png"
                        width: 14
                        height: 14
                        sourceSize.width: 14
                        sourceSize.height: 14
                        Layout.preferredWidth: 14
                        Layout.preferredHeight: 14
                        Layout.alignment: Qt.AlignVCenter
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                        mipmap: true
                    }
                    Text {
                        text: "PICAUDIO"
                        font.pixelSize: 11
                        font.bold: true
                        font.letterSpacing: 1.5
                        color: Theme.textPrimary
                        Layout.alignment: Qt.AlignVCenter
                    }
                }

                Rectangle {
                    width: 1
                    height: 14
                    Layout.preferredWidth: 1
                    Layout.preferredHeight: 14
                    Layout.alignment: Qt.AlignVCenter
                    color: Theme.panelBorder
                    Layout.leftMargin: 6
                    Layout.rightMargin: 6
                }

                // Desktop Menu Bar Component
                AppMenuBar {
                    Layout.alignment: Qt.AlignVCenter
                    Layout.preferredHeight: 22
                    showLeftPanel: root.showLeftPanel
                    showColumnBrowser: root.showColumnBrowser
                    showRightPanel: root.showRightPanel
                    rightPanelTab: root.rightPanelTab
                    showEqualizer: root.showEqualizer
                    showStatusBar: root.showStatusBar
                    isMiniPlayer: root.isMiniPlayer
                    isFullscreen: root.visibility === Window.FullScreen

                    onShowLeftPanelChanged: root.showLeftPanel = showLeftPanel
                    onShowColumnBrowserChanged: root.showColumnBrowser = showColumnBrowser
                    onShowRightPanelChanged: root.showRightPanel = showRightPanel
                    onRightPanelTabChanged: root.rightPanelTab = rightPanelTab
                    onRequestToggleEqualizer: root.showEqualizer = !root.showEqualizer
                    onShowStatusBarChanged: root.showStatusBar = showStatusBar

                    onRequestOpenAudioFile: openAudioFileDialog.open()
                    onRequestOpenFolder: openMusicFolderDialog.open()
                    onRequestOpenUrl: openUrlDialog.open()
                    onRequestOpenCueSheet: openCueSheetDialog.open()
                    onRequestAddFolderToLibrary: folderDialog.open()
                    onRequestExportView: exportViewDialog.open()
                    onRequestPreferences: (cat) => {
                        preferencesDialog.activeCategory = cat;
                        preferencesDialog.open();
                    }
                    onRequestEditTags: {
                        var sel = bridge.trackModel.getSelectedTracks();
                        if (sel.length > 0) {
                            tagEditorDialog.targetTracks = sel.length > 1 ? sel : [];
                            tagEditorDialog.targetTrack = sel[0];
                        } else {
                            tagEditorDialog.targetTracks = [];
                            tagEditorDialog.targetTrack = null;
                        }
                        tagEditorDialog.open();
                    }
                    onRequestNewPlaylist: (isSmart) => {
                        if (isSmart) {
                            smartPlaylistRuleDialog.playlistName = "";
                            smartPlaylistRuleDialog.open();
                        } else {
                            newPlaylistDialog.isSmart = false;
                            newPlaylistDialog.open();
                        }
                    }
                    onRequestLibraryHealth: libraryHealthDialog.open()
                    onRequestArrangePanels: arrangePanelsDialogInstance.open()
                    onRequestImportPlaylist: importPlaylistDialog.open()
                    onRequestSleepTimer: sleepTimerDialog.open()
                    onRequestShortcuts: shortcutsDialog.open()
                    onRequestDiagnostics: diagnosticsDialog.open()
                    onRequestAbout: aboutDialog.open()
                    onRequestFocusSearch: topToolbar.focusSearch()
                    onRequestClearSearch: topToolbar.clearSearch()
                    onRequestToggleFullscreen: root.toggleFullscreen()
                    onRequestToggleMiniPlayer: root.toggleMiniPlayer()
                    onRequestHideWindow: root.hide()
                }

                Item { Layout.fillWidth: true }
            }
        }

        // 1px divider
        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.panelBorder }

        // Toolbar
        TopToolbar {
            id: topToolbar
            showLeftPanel: root.showLeftPanel
            showRightPanel: root.showRightPanel
            onRequestOpenFolderDialog: folderDialog.open()
            onRequestPreferences: preferencesDialog.open()
            onRequestArrangePanels: arrangePanelsDialogInstance.open()
            onRequestCustomizeView: viewConfigDialogInstance.open()
            onToggleLeftPanel: root.showLeftPanel = !root.showLeftPanel
            onToggleRightPanel: root.showRightPanel = !root.showRightPanel
        }

        // 1px divider
        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.panelBorder }

        // Tabs Bar Header Component (Issue #74)
        TabsBar {
            id: headerTabsBar
            Layout.fillWidth: true
        }

        // 1px divider
        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.panelBorder }

        // Top Panel Zone (Issue #72)
        Rectangle {
            id: topPanelZone
            Layout.fillWidth: true
            Layout.preferredHeight: bridge.layoutModel.topPanelHeight
            visible: bridge.layoutModel.effectiveTopPanelVisible
            color: Theme.surface
            border.color: Theme.panelBorder
            border.width: 1
            clip: true

            WaveVisualizer {
                anchors.fill: parent
                anchors.margins: 4
            }
        }

        // 1px divider
        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.panelBorder }

        // Main workspace
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // Left panel
            LeftSidebar {
                activeNavSection: root.activeNavSection
                activePlaylistName: root.activePlaylistName
                visible: root.showLeftPanel
                onNavSectionSelected: (nav, mode) => {
                    root.activeNavSection = nav;
                    if (nav !== 5 && nav !== 6) {
                        root.activePlaylistName = "";
                    } else if (nav === 6) {
                        root.activePlaylistName = "Favorites";
                    }
                }
                onPlaylistSelected: (name) => {
                    root.activeNavSection = 5;
                    root.activePlaylistName = name;
                }
                onRequestSwitchToNowPlaying: {
                    root.activePlaylistName = "";
                    root.switchToNowPlaying();
                }
                onRequestShowColumnBrowser: (show) => root.showColumnBrowser = show
                onRequestAddMusicFolder: folderDialog.open()
                onRequestFolderContextMenu: (path) => {
                    folderContextMenu.targetFolderPath = path;
                    folderContextMenu.popup();
                }
                onRequestPlaylistContextMenu: (name, isSmart) => {
                    playlistContextMenu.targetPlaylistName = name;
                    playlistContextMenu.targetPlaylistIsSmart = isSmart;
                    playlistContextMenu.popup();
                }
                onRequestExplorerContextMenu: explorerContextMenu.popup()
            }

            // Resizable splitter divider: Left Library Explorer sidebar
            Item {
                Layout.preferredWidth: 5
                Layout.fillHeight: true
                visible: root.showLeftPanel

                Rectangle {
                    anchors.centerIn: parent
                    width: 1
                    height: parent.height
                    color: leftSplitMouse.containsMouse ? Theme.accent : Theme.panelBorder
                }

                MouseArea {
                    id: leftSplitMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.SplitHCursor
                    property real startRootX: 0
                    property int startWidth: 0
                    onPressed: (mouse) => {
                        var pt = mapToItem(root.contentItem, mouse.x, mouse.y);
                        startRootX = pt.x;
                        startWidth = bridge.leftPanelWidth;
                    }
                    onPositionChanged: (mouse) => {
                        if (pressed) {
                            var pt = mapToItem(root.contentItem, mouse.x, mouse.y);
                            var delta = pt.x - startRootX;
                            bridge.leftPanelWidth = Math.max(140, Math.min(450, Math.round(startWidth + delta)));
                        }
                    }
                    onDoubleClicked: bridge.leftPanelWidth = 210
                }
            }

            // Left Main Panel (Zone 4)
            ThumbnailBrowserView {
                id: leftMainPanelZone
                Layout.preferredWidth: bridge.layoutModel.leftMainPanelWidth
                Layout.fillHeight: true
                visible: bridge.layoutModel.effectiveLeftMainPanelVisible
            }

            // Center panel
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: Theme.surface

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    // Sub-toolbar
                    ViewSubToolbar {
                        showColumnBrowser: bridge.layoutModel ? bridge.layoutModel.columnBrowserVisible : false
                        activeLetterFilter: root.activeLetterFilter
                        onToggleColumnBrowser: {
                            if (bridge.layoutModel) {
                                bridge.layoutModel.columnBrowserVisible = !bridge.layoutModel.columnBrowserVisible;
                            }
                        }
                        onLetterFilterSelected: (letter) => {
                            root.activeLetterFilter = letter;
                            bridge.filterByLetter(letter);
                            if (bridge.tabModel) {
                                bridge.tabModel.updateActiveJumpBarLetter(letter);
                            }
                        }
                        onRequestCustomizeView: viewConfigDialogInstance.open()
                    }

                    // Main workspace host (dynamically renders active tab view)
                    MainWorkspaceHost {
                        id: mainWorkspaceHost
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        onRequestColumnHeaderContextMenu: (colKey, colTitle) => {
                            columnHeaderContextMenu.targetColumnKey = colKey;
                            columnHeaderContextMenu.targetColumnTitle = colTitle;
                            columnHeaderContextMenu.popup();
                        }
                        onRequestViewportContextMenu: {
                            viewportContextMenu.popup();
                        }
                        onRequestTrackContextMenu: (targetTracks, targetTrack, targetFilePath, targetTitle, targetArtist, isPlView, plName) => {
                            trackContextMenu.targetTracks = targetTracks;
                            trackContextMenu.targetTrack = targetTrack;
                            trackContextMenu.targetFilePath = targetFilePath;
                            trackContextMenu.targetTitle = targetTitle;
                            trackContextMenu.targetArtist = targetArtist;
                            trackContextMenu.isPlaylistView = isPlView;
                            trackContextMenu.currentPlaylistName = plName;
                            trackContextMenu.popup();
                        }
                        onRequestAlbumContextMenu: (title, artist) => {
                            albumContextMenu.targetAlbumTitle = title;
                            albumContextMenu.targetAlbumArtist = artist;
                            albumContextMenu.popup();
                        }
                        onRequestExpandAlbum: (album, artist) => {
                            root.switchToExpandedView(album, artist);
                        }
                    }
                }
            }

            // Right Main Panel (Zone 6)
            Rectangle {
                id: rightMainPanelZone
                Layout.preferredWidth: bridge.layoutModel.rightMainPanelWidth
                Layout.fillHeight: true
                visible: bridge.layoutModel.effectiveRightMainPanelVisible
                color: Theme.surface
                border.color: Theme.panelBorder
                border.width: 1
                clip: true

                Text {
                    anchors.centerIn: parent
                    anchors.margins: 12
                    width: parent.width - 24
                    text: (bridge.currentTrackLyrics && bridge.currentTrackLyrics.length > 0) ? bridge.currentTrackLyrics : qsTr("Lyrics / Track Details")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeSmall
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                }
            }

            // Resizable splitter divider: Right Inspector panel
            Item {
                Layout.preferredWidth: 5
                Layout.fillHeight: true
                visible: root.showRightPanel

                Rectangle {
                    anchors.centerIn: parent
                    width: 1
                    height: parent.height
                    color: rightSplitMouse.containsMouse ? Theme.accent : Theme.panelBorder
                }

                MouseArea {
                    id: rightSplitMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.SplitHCursor
                    property real startRootX: 0
                    property int startWidth: 0
                    onPressed: (mouse) => {
                        var pt = mapToItem(root.contentItem, mouse.x, mouse.y);
                        startRootX = pt.x;
                        startWidth = bridge.rightPanelWidth;
                    }
                    onPositionChanged: (mouse) => {
                        if (pressed) {
                            var pt = mapToItem(root.contentItem, mouse.x, mouse.y);
                            var delta = pt.x - startRootX;
                            bridge.rightPanelWidth = Math.max(200, Math.min(450, Math.round(startWidth - delta)));
                        }
                    }
                    onDoubleClicked: bridge.rightPanelWidth = 270
                }
            }

            // Right panel
            RightInspectorPanel {
                currentTab: root.rightPanelTab
                visible: root.showRightPanel
                onTabChanged: (tab) => root.rightPanelTab = tab
            }
        }

        // 1px divider
        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.panelBorder }

        // Transport controls
        PlayerDeck {
            showEqualizer: root.showEqualizer
            onRequestToggleEqualizer: root.showEqualizer = !root.showEqualizer
            onRequestSwitchToNowPlaying: root.switchToNowPlaying()
            onRequestToggleFullscreen: root.toggleFullscreen()
            onRequestSeekBarContextMenu: seekBarContextMenu.popup()
            onRequestCodecContextMenu: codecContextMenu.popup()
            onRequestVolumeContextMenu: volumeContextMenu.popup()
            onRequestSleepTimer: sleepTimerDialog.open()
            onRequestDiagnostics: diagnosticsDialog.open()
        }

        // Status bar
        StatusBar {
            visible: showStatusBar
        }
    }

    // Mini-player mode layout
    MiniPlayer {
        id: miniPlayerContainer
        anchors.fill: parent
        visible: isMiniPlayer
        stayOnTop: root.stayOnTop
        onRequestToggleStayOnTop: root.stayOnTop = !root.stayOnTop
        onRequestRestore: root.toggleMiniPlayer()
        onRequestMove: root.startSystemMove()
        onRequestResize: (edge) => {
            if (typeof root.startSystemResize === "function") {
                root.startSystemResize(edge);
            }
        }
        onRequestContextMenu: {
            miniPlayerContextMenu.stayOnTop = root.stayOnTop;
            miniPlayerContextMenu.currentOpacity = root.opacity;
            miniPlayerContextMenu.popup();
        }
    }

    // Equalizer Overlay Dialog
    EqualizerOverlay {
        id: equalizerOverlay
        z: 100
        onClosed: {
            root.showEqualizer = false;
        }
    }

    // In-app non-blocking Toast Banner
    ToastBanner {
        id: toastBanner
    }
}
