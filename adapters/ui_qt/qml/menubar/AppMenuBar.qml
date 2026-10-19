import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

RowLayout {
    id: root
    spacing: 2

    // Dialog & external action signals
    signal requestOpenAudioFile()
    signal requestOpenFolder()
    signal requestOpenUrl()
    signal requestOpenCueSheet()
    signal requestAddFolderToLibrary()
    signal requestExportView()
    signal requestPreferences(int category)
    signal requestEditTags()
    signal requestNewPlaylist(bool isSmart)
    signal requestImportPlaylist()
    signal requestSleepTimer()
    signal requestShortcuts()
    signal requestDiagnostics()
    signal requestAbout()
    signal requestFocusSearch()
    signal requestClearSearch()
    signal requestToggleFullscreen()
    signal requestToggleMiniPlayer()
    signal requestHideWindow()
    signal requestSwitchViewMode(int mode)

    // Two-way UI state properties
    property bool showLeftPanel: true
    property bool showColumnBrowser: false
    property bool showRightPanel: true
    property int rightPanelTab: 0
    property bool showEqualizer: false
    property bool showStatusBar: true
    property int mainViewMode: 1
    property bool isMiniPlayer: false
    property bool isFullscreen: false

    function formatTimestamp(ms) {
        if (ms < 0) return "--:--";
        var totalSec = Math.floor(ms / 1000);
        var mins = Math.floor(totalSec / 60);
        var secs = totalSec % 60;
        return mins + ":" + (secs < 10 ? "0" : "") + secs;
    }

    function zoomIn() {
        if (Theme.uiScale < 1.5) {
            Theme.uiScale = Math.min(1.5, Math.round((Theme.uiScale + 0.1) * 10) / 10);
        }
    }

    function zoomOut() {
        if (Theme.uiScale > 0.7) {
            Theme.uiScale = Math.max(0.7, Math.round((Theme.uiScale - 0.1) * 10) / 10);
        }
    }

    function zoomReset() {
        Theme.uiScale = 1.0;
    }

    // File Menu Button
    Rectangle {
        width: fileMenuText.implicitWidth + 14
        height: 22
        radius: 3
        color: fileMenuMouse.containsMouse ? Theme.surfaceElevated : "transparent"
        Text {
            id: fileMenuText
            anchors.centerIn: parent
            text: "File"
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.textSecondary
        }
        MouseArea {
            id: fileMenuMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: fileMenu.open()
        }
        Menu {
            id: fileMenu
            y: parent.height
            MenuItem {
                text: "Open Audio File... (Ctrl+O)"
                onTriggered: root.requestOpenAudioFile()
            }
            MenuItem {
                text: "Open Folder... (Ctrl+Shift+O)"
                onTriggered: root.requestOpenFolder()
            }
            MenuItem {
                text: "Open URL / Network Stream... (Ctrl+U)"
                onTriggered: root.requestOpenUrl()
            }
            MenuItem {
                text: "Open CUE Sheet..."
                onTriggered: root.requestOpenCueSheet()
            }
            MenuSeparator {}
            MenuItem {
                text: "Add Folder to Library..."
                onTriggered: root.requestAddFolderToLibrary()
            }
            MenuItem {
                text: "Export Active View / Playlist... (Ctrl+E)"
                onTriggered: root.requestExportView()
            }
            MenuSeparator {}
            MenuItem {
                text: "Preferences & Settings... (Ctrl+,)"
                onTriggered: root.requestPreferences(0)
            }
            MenuSeparator {}
            MenuItem {
                text: "Minimize to System Tray (Ctrl+W)"
                onTriggered: root.requestHideWindow()
            }
            MenuItem {
                text: "Exit (Ctrl+Q)"
                onTriggered: Qt.quit()
            }
        }
    }

    // Edit Menu Button
    Rectangle {
        width: editMenuText.implicitWidth + 14
        height: 22
        radius: 3
        color: editMenuMouse.containsMouse ? Theme.surfaceElevated : "transparent"
        Text {
            id: editMenuText
            anchors.centerIn: parent
            text: "Edit"
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.textSecondary
        }
        MouseArea {
            id: editMenuMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: editMenu.open()
        }
        Menu {
            id: editMenu
            y: parent.height
            MenuItem {
                text: bridge.undoActionName.length > 0 ? ("Undo " + bridge.undoActionName + " (Ctrl+Z)") : "Undo (Ctrl+Z)"
                enabled: bridge.canUndo
                onTriggered: bridge.undo()
            }
            MenuItem {
                text: bridge.redoActionName.length > 0 ? ("Redo " + bridge.redoActionName + " (Ctrl+Y)") : "Redo (Ctrl+Y)"
                enabled: bridge.canRedo
                onTriggered: bridge.redo()
            }
            MenuSeparator {}
            MenuItem {
                text: "Select All (Ctrl+A)"
                onTriggered: bridge.trackModel.selectAll()
            }
            MenuItem {
                text: "Invert Selection (Ctrl+Shift+A)"
                onTriggered: bridge.trackModel.invertSelection()
            }
            MenuItem {
                text: "Clear Selection (Esc)"
                enabled: bridge.trackModel.selectedCount > 0
                onTriggered: bridge.trackModel.clearSelection()
            }
            MenuSeparator {}
            MenuItem {
                text: "Find / Instant Search (Ctrl+F)"
                onTriggered: root.requestFocusSearch()
            }
            MenuItem {
                text: "Clear Search Filter (Shift+Esc)"
                onTriggered: root.requestClearSearch()
            }
            MenuSeparator {}
            MenuItem {
                text: "Edit Track Tags... (Ctrl+T)"
                enabled: bridge.trackModel.selectedCount > 0
                onTriggered: root.requestEditTags()
            }
            MenuItem {
                text: "Delete Selected Tracks (Delete)"
                enabled: bridge.trackModel.selectedCount > 0
                onTriggered: {
                    var ids = bridge.trackModel.getSelectedTrackIds();
                    if (ids.length > 0) {
                        bridge.deleteSelectedTracks(ids);
                    }
                }
            }
        }
    }

    // View Menu Button
    Rectangle {
        width: viewMenuText.implicitWidth + 14
        height: 22
        radius: 3
        color: viewMenuMouse.containsMouse ? Theme.surfaceElevated : "transparent"
        Text {
            id: viewMenuText
            anchors.centerIn: parent
            text: "View"
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.textSecondary
        }
        MouseArea {
            id: viewMenuMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: viewMenu.open()
        }
        Menu {
            id: viewMenu
            y: parent.height
            MenuItem {
                text: (root.showLeftPanel ? "Hide" : "Show") + " Left Navigator (Ctrl+1)"
                onTriggered: root.showLeftPanel = !root.showLeftPanel
            }
            MenuItem {
                text: (root.showColumnBrowser ? "Hide" : "Show") + " Category Filters (Ctrl+2)"
                onTriggered: root.showColumnBrowser = !root.showColumnBrowser
            }
            MenuItem {
                text: (root.showRightPanel && root.rightPanelTab === 0 ? "Hide" : "Show") + " Audio Specs Inspector (Ctrl+3)"
                onTriggered: {
                    if (!root.showRightPanel || root.rightPanelTab !== 0) {
                        root.showRightPanel = true;
                        root.rightPanelTab = 0;
                    } else {
                        root.showRightPanel = false;
                    }
                }
            }
            MenuItem {
                text: (root.showRightPanel && root.rightPanelTab === 1 ? "Hide" : "Show") + " Up Next Play Queue (Ctrl+4)"
                onTriggered: {
                    if (!root.showRightPanel || root.rightPanelTab !== 1) {
                        root.showRightPanel = true;
                        root.rightPanelTab = 1;
                    } else {
                        root.showRightPanel = false;
                    }
                }
            }
            MenuItem {
                text: (root.showEqualizer ? "Hide" : "Show") + " Parametric Equalizer (Ctrl+5)"
                onTriggered: root.showEqualizer = !root.showEqualizer
            }
            MenuItem {
                text: (root.showStatusBar ? "Hide" : "Show") + " Bottom Status Bar"
                checkable: true
                checked: root.showStatusBar
                onTriggered: root.showStatusBar = !root.showStatusBar
            }
            MenuSeparator {}
            Menu {
                title: "View Modes"
                MenuItem {
                    text: "Track Details Table (Alt+1)"
                    checkable: true
                    checked: root.mainViewMode === 1
                    onTriggered: root.requestSwitchViewMode(1)
                }
                MenuItem {
                    text: "Album Grid (Alt+2)"
                    checkable: true
                    checked: root.mainViewMode === 0
                    onTriggered: root.requestSwitchViewMode(0)
                }
                MenuItem {
                    text: "Album Details & Tracks (Alt+3)"
                    checkable: true
                    checked: root.mainViewMode === 2
                    onTriggered: root.requestSwitchViewMode(2)
                }
                MenuItem {
                    text: "Now Playing View (Alt+4)"
                    checkable: true
                    checked: root.mainViewMode === 4
                    onTriggered: root.requestSwitchViewMode(4)
                }
                MenuItem {
                    text: "Listening History (Alt+5)"
                    checkable: true
                    checked: root.mainViewMode === 3
                    onTriggered: root.requestSwitchViewMode(3)
                }
                MenuSeparator {}
                MenuItem {
                    text: "Group Albums with Guest/Featuring Artists"
                    checkable: true
                    checked: bridge.groupAlbumsByTitle
                    onTriggered: bridge.setGroupAlbumsByTitle(!bridge.groupAlbumsByTitle)
                }
            }
            MenuItem {
                text: (root.isMiniPlayer ? "Restore Full Player" : "Mini-Player Mode") + " (Ctrl+Shift+M / F10)"
                onTriggered: root.requestToggleMiniPlayer()
            }
            MenuItem {
                text: (root.isFullscreen ? "Exit Fullscreen" : "Fullscreen") + " (F11)"
                onTriggered: root.requestToggleFullscreen()
            }
            MenuSeparator {}
            Menu {
                title: "Zoom UI"
                MenuItem {
                    text: "Zoom In (Ctrl++)"
                    onTriggered: root.zoomIn()
                }
                MenuItem {
                    text: "Zoom Out (Ctrl+-)"
                    onTriggered: root.zoomOut()
                }
                MenuItem {
                    text: "Reset Zoom (100%) (Ctrl+0)"
                    onTriggered: root.zoomReset()
                }
                MenuSeparator {}
                MenuItem {
                    text: "75%"
                    checkable: true
                    checked: Math.abs(Theme.uiScale - 0.75) < 0.04
                    onTriggered: Theme.uiScale = 0.75
                }
                MenuItem {
                    text: "90%"
                    checkable: true
                    checked: Math.abs(Theme.uiScale - 0.9) < 0.04
                    onTriggered: Theme.uiScale = 0.9
                }
                MenuItem {
                    text: "100% (Standard)"
                    checkable: true
                    checked: Math.abs(Theme.uiScale - 1.0) < 0.04
                    onTriggered: Theme.uiScale = 1.0
                }
                MenuItem {
                    text: "110%"
                    checkable: true
                    checked: Math.abs(Theme.uiScale - 1.1) < 0.04
                    onTriggered: Theme.uiScale = 1.1
                }
                MenuItem {
                    text: "125%"
                    checkable: true
                    checked: Math.abs(Theme.uiScale - 1.25) < 0.04
                    onTriggered: Theme.uiScale = 1.25
                }
                MenuItem {
                    text: "150%"
                    checkable: true
                    checked: Math.abs(Theme.uiScale - 1.5) < 0.04
                    onTriggered: Theme.uiScale = 1.5
                }
            }
            Menu {
                id: themesQuickMenu
                title: "Themes Quick Switch"
                Instantiator {
                    model: Theme.availableThemes
                    onObjectAdded: (index, object) => themesQuickMenu.insertItem(index, object)
                    onObjectRemoved: (index, object) => themesQuickMenu.removeItem(object)
                    delegate: MenuItem {
                        text: (modelData.name || modelData.id) + (Theme.themeId === modelData.id ? "  ✓" : "")
                        onTriggered: Theme.themeId = modelData.id
                    }
                }
                MenuSeparator {}
                MenuItem {
                    text: "Manage Themes..."
                    onTriggered: root.requestPreferences(1)
                }
            }
        }
    }

    // Playback Menu Button
    Rectangle {
        width: playbackMenuText.implicitWidth + 14
        height: 22
        radius: 3
        color: playbackMenuMouse.containsMouse ? Theme.surfaceElevated : "transparent"
        Text {
            id: playbackMenuText
            anchors.centerIn: parent
            text: "Playback"
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.textSecondary
        }
        MouseArea {
            id: playbackMenuMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: playbackMenu.open()
        }
        Menu {
            id: playbackMenu
            y: parent.height
            MenuItem {
                text: bridge.isPlaying ? "Pause (Space)" : "Play (Space)"
                onTriggered: bridge.togglePlayPause()
            }
            MenuItem {
                text: "Stop (Ctrl+.)"
                onTriggered: bridge.stop()
            }
            MenuItem {
                text: "Stop After Current Track (Shift+Space)"
                checkable: true
                checked: bridge.stopAfterCurrentTrack
                onTriggered: bridge.toggleStopAfterCurrentTrack()
            }
            MenuItem {
                text: "Next Track (Ctrl+Right)"
                onTriggered: bridge.nextTrack()
            }
            MenuItem {
                text: "Previous Track (Ctrl+Left)"
                onTriggered: bridge.previousTrack()
            }
            MenuSeparator {}
            Menu {
                title: "Seeking"
                MenuItem {
                    text: "Short Seek Backward 5s (Left)"
                    onTriggered: bridge.seek(bridge.positionMs - 5000)
                }
                MenuItem {
                    text: "Short Seek Forward 5s (Right)"
                    onTriggered: bridge.seek(bridge.positionMs + 5000)
                }
                MenuItem {
                    text: "Long Seek Backward 30s (Shift+Left)"
                    onTriggered: bridge.seek(bridge.positionMs - 30000)
                }
                MenuItem {
                    text: "Long Seek Forward 30s (Shift+Right)"
                    onTriggered: bridge.seek(bridge.positionMs + 30000)
                }
                MenuSeparator {}
                MenuItem {
                    text: "Seek to Track Beginning (Home)"
                    onTriggered: bridge.seek(0)
                }
                MenuItem {
                    text: "Seek to Track End (End)"
                    onTriggered: bridge.seek(bridge.durationMs)
                }
            }
            MenuSeparator {}
            MenuItem {
                text: "Volume Up 5% (Ctrl+Up)"
                onTriggered: bridge.setVolume(bridge.volume + 0.05)
            }
            MenuItem {
                text: "Volume Down 5% (Ctrl+Down)"
                onTriggered: bridge.setVolume(bridge.volume - 0.05)
            }
            MenuItem {
                text: "Mute Toggle (Ctrl+M)"
                checkable: true
                checked: bridge.isMuted
                onTriggered: bridge.toggleMute()
            }
            MenuSeparator {}
            Menu {
                title: "Playback Speed"
                MenuItem {
                    text: "0.5x"
                    checkable: true
                    checked: Math.abs(bridge.playbackRate - 0.5) < 0.05
                    onTriggered: bridge.setPlaybackRate(0.5)
                }
                MenuItem {
                    text: "0.75x"
                    checkable: true
                    checked: Math.abs(bridge.playbackRate - 0.75) < 0.05
                    onTriggered: bridge.setPlaybackRate(0.75)
                }
                MenuItem {
                    text: "1.0x (Normal)"
                    checkable: true
                    checked: Math.abs(bridge.playbackRate - 1.0) < 0.05
                    onTriggered: bridge.setPlaybackRate(1.0)
                }
                MenuItem {
                    text: "1.25x"
                    checkable: true
                    checked: Math.abs(bridge.playbackRate - 1.25) < 0.05
                    onTriggered: bridge.setPlaybackRate(1.25)
                }
                MenuItem {
                    text: "1.5x"
                    checkable: true
                    checked: Math.abs(bridge.playbackRate - 1.5) < 0.05
                    onTriggered: bridge.setPlaybackRate(1.5)
                }
                MenuItem {
                    text: "2.0x"
                    checkable: true
                    checked: Math.abs(bridge.playbackRate - 2.0) < 0.05
                    onTriggered: bridge.setPlaybackRate(2.0)
                }
            }
            Menu {
                title: "A-B Looping"
                MenuItem {
                    text: "Set Loop Point A (" + (bridge.loopPointA >= 0 ? root.formatTimestamp(bridge.loopPointA) : "--:--") + ")  ([)"
                    onTriggered: bridge.setLoopPointA()
                }
                MenuItem {
                    text: "Set Loop Point B (" + (bridge.loopPointB >= 0 ? root.formatTimestamp(bridge.loopPointB) : "--:--") + ")  (])"
                    onTriggered: bridge.setLoopPointB()
                }
                MenuItem {
                    text: "Clear Loop (\\)"
                    enabled: bridge.isLoopActive || bridge.loopPointA >= 0 || bridge.loopPointB >= 0
                    onTriggered: bridge.clearLoop()
                }
            }
            MenuSeparator {}
            MenuItem {
                text: "Cycle Shuffle Mode (Ctrl+S)"
                onTriggered: bridge.cycleShuffleMode()
            }
            MenuItem {
                text: "Cycle Repeat Mode (Ctrl+R)"
                onTriggered: bridge.cycleRepeatMode()
            }
            MenuSeparator {}
            MenuItem {
                text: "Play All Tracks"
                onTriggered: bridge.playAll()
            }
            MenuItem {
                text: "Shuffle All Tracks"
                onTriggered: bridge.shuffleAll()
            }
        }
    }

    // Library Menu Button
    Rectangle {
        width: libraryMenuText.implicitWidth + 14
        height: 22
        radius: 3
        color: libraryMenuMouse.containsMouse ? Theme.surfaceElevated : "transparent"
        Text {
            id: libraryMenuText
            anchors.centerIn: parent
            text: "Library"
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.textSecondary
        }
        MouseArea {
            id: libraryMenuMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: libraryMenu.open()
        }
        Menu {
            id: libraryMenu
            y: parent.height
            MenuItem {
                text: "Rescan All Monitored Folders (F5)"
                onTriggered: bridge.rescanAllMonitoredFolders()
            }
            MenuItem {
                text: "Quick Incremental Scan (Ctrl+F5)"
                onTriggered: bridge.incrementalQuickScan()
            }
            MenuItem {
                text: "Manage Monitored Folders..."
                onTriggered: root.requestPreferences(3)
            }
            MenuItem {
                text: "Group Albums with Guest/Featuring Artists"
                checkable: true
                checked: bridge.groupAlbumsByTitle
                onTriggered: bridge.setGroupAlbumsByTitle(!bridge.groupAlbumsByTitle)
            }
            MenuSeparator {}
            MenuItem {
                text: "Create New Playlist... (Ctrl+N)"
                onTriggered: root.requestNewPlaylist(false)
            }
            MenuItem {
                text: "Create Smart Playlist... (Ctrl+Shift+N)"
                onTriggered: root.requestNewPlaylist(true)
            }
            MenuItem {
                text: "Import Playlist..."
                onTriggered: root.requestImportPlaylist()
            }
            MenuItem {
                text: "Export Active View / Playlist... (Ctrl+E)"
                onTriggered: root.requestExportView()
            }
            MenuSeparator {}
            MenuItem {
                text: "Deduplicate Library Tracks"
                onTriggered: bridge.deduplicateTracks()
            }
            MenuItem {
                text: "Check for Dead / Broken Files"
                onTriggered: bridge.purgeMissingTracks()
            }
            MenuItem {
                text: "Clear Cover Art Cache"
                onTriggered: bridge.clearCoverArtCache()
            }
            MenuSeparator {}
            MenuItem {
                text: "Clear Entire Library..."
                onTriggered: bridge.clearLibrary()
            }
        }
    }

    // Tools Menu Button
    Rectangle {
        width: toolsMenuText.implicitWidth + 14
        height: 22
        radius: 3
        color: toolsMenuMouse.containsMouse ? Theme.surfaceElevated : "transparent"
        Text {
            id: toolsMenuText
            anchors.centerIn: parent
            text: "Tools"
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.textSecondary
        }
        MouseArea {
            id: toolsMenuMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: toolsMenu.open()
        }
        Menu {
            id: toolsMenu
            y: parent.height
            MenuItem {
                text: "Metadata Tag Editor... (Ctrl+T)"
                onTriggered: root.requestEditTags()
            }
            MenuItem {
                text: (root.showEqualizer ? "Hide" : "Show") + " Parametric Equalizer & DSP (Ctrl+Shift+E)"
                onTriggered: root.showEqualizer = !root.showEqualizer
            }
            MenuItem {
                text: "Playback Sleep Timer..."
                onTriggered: root.requestSleepTimer()
            }
            MenuSeparator {}
            Menu {
                id: audioDeviceMenu
                title: "Audio Output Device"
                Instantiator {
                    model: bridge.availableAudioDevices
                    onObjectAdded: (index, object) => audioDeviceMenu.insertItem(index, object)
                    onObjectRemoved: (index, object) => audioDeviceMenu.removeItem(object)
                    delegate: MenuItem {
                        text: modelData + (bridge.currentAudioDevice === modelData ? "  ✓" : "")
                        checkable: true
                        checked: bridge.currentAudioDevice === modelData
                        onTriggered: bridge.setCurrentAudioDevice(modelData)
                    }
                }
            }
            MenuSeparator {}
            MenuItem {
                text: "Preferences & Settings... (Ctrl+,)"
                onTriggered: root.requestPreferences(0)
            }
            MenuItem {
                text: "Manage Plugins & Extensions..."
                onTriggered: root.requestPreferences(5)
            }
        }
    }

    // Help Menu Button
    Rectangle {
        width: helpMenuText.implicitWidth + 14
        height: 22
        radius: 3
        color: helpMenuMouse.containsMouse ? Theme.surfaceElevated : "transparent"
        Text {
            id: helpMenuText
            anchors.centerIn: parent
            text: "Help"
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.textSecondary
        }
        MouseArea {
            id: helpMenuMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: helpMenu.open()
        }
        Menu {
            id: helpMenu
            y: parent.height
            MenuItem {
                text: "Documentation & User Guide (F1)"
                onTriggered: Qt.openUrlExternally("https://github.com/ShamalLakshan/PicAudio")
            }
            MenuItem {
                text: "Keyboard Shortcuts Cheat Sheet (Ctrl+/)"
                onTriggered: root.requestShortcuts()
            }
            MenuItem {
                text: "Audio Pipeline Diagnostics..."
                onTriggered: root.requestDiagnostics()
            }
            MenuItem {
                text: "Open Application Log File"
                onTriggered: Qt.openUrlExternally("file://" + bridge.getLogFilePath())
            }
            MenuSeparator {}
            MenuItem {
                text: "Report an Issue / GitHub..."
                onTriggered: Qt.openUrlExternally("https://github.com/ShamalLakshan/PicAudio/issues")
            }
            MenuSeparator {}
            MenuItem {
                text: "About PicAudio"
                onTriggered: root.requestAbout()
            }
        }
    }
}
