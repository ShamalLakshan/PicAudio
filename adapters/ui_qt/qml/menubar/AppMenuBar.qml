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
    signal requestToggleEqualizer()
    signal requestVisualizerDialog()
    signal requestLibraryHealth()
    signal requestArrangePanels()

    // Two-way UI state properties
    property bool showLeftPanel: true
    property bool showColumnBrowser: false
    property bool showRightPanel: true
    property int rightPanelTab: 0
    property bool showEqualizer: false
    property bool showStatusBar: true
    property bool isMiniPlayer: false
    property bool isFullscreen: false

    readonly property bool isAnyMenuOpen: (fileMenu && fileMenu.visible) ||
                                          (editMenu && editMenu.visible) ||
                                          (viewMenu && viewMenu.visible) ||
                                          (tabsMenu && tabsMenu.visible) ||
                                          (playbackMenu && playbackMenu.visible) ||
                                          (libraryMenu && libraryMenu.visible) ||
                                          (toolsMenu && toolsMenu.visible) ||
                                          (helpMenu && helpMenu.visible)

    function closeAllMenus() {
        if (fileMenu.visible) fileMenu.close();
        if (editMenu.visible) editMenu.close();
        if (viewMenu.visible) viewMenu.close();
        if (tabsMenu.visible) tabsMenu.close();
        if (playbackMenu.visible) playbackMenu.close();
        if (libraryMenu.visible) libraryMenu.close();
        if (toolsMenu.visible) toolsMenu.close();
        if (helpMenu.visible) helpMenu.close();
    }

    component MenuBarPopup: Menu {
        id: menuRoot
        topPadding: Math.round(4 * Theme.uiScale)
        bottomPadding: Math.round(4 * Theme.uiScale)
        leftPadding: Math.round(4 * Theme.uiScale)
        rightPadding: Math.round(4 * Theme.uiScale)

        delegate: MenuBarItem {}

        background: Rectangle {
            implicitWidth: Math.round(250 * Theme.uiScale)
            color: Theme.surfaceElevated
            border.color: Theme.panelBorder
            border.width: 1
            radius: Theme.cornerRadiusSmall
        }
    }

    component MenuBarItem: MenuItem {
        id: mItem
        implicitHeight: visible ? Math.round(26 * Theme.uiScale) : 0
        implicitWidth: Math.max(Math.round(250 * Theme.uiScale), itemLabel.implicitWidth + (mItem.checkable ? Math.round(20 * Theme.uiScale) : 0) + (mItem.subMenu ? Math.round(20 * Theme.uiScale) : 0) + Math.round(28 * Theme.uiScale))
        arrow: Item { visible: false; width: 0; height: 0 }
        indicator: Item { visible: false; width: 0; height: 0 }

        HoverHandler {
            cursorShape: Qt.PointingHandCursor
        }

        contentItem: RowLayout {
            anchors.fill: parent
            anchors.leftMargin: Math.round(10 * Theme.uiScale)
            anchors.rightMargin: Math.round(10 * Theme.uiScale)
            spacing: Math.round(6 * Theme.uiScale)

            Text {
                visible: mItem.checkable
                text: mItem.checked ? "✓" : ""
                font.pixelSize: Math.round(11 * Theme.uiScale)
                font.bold: true
                font.family: Theme.fontFamily
                color: Theme.accent
                Layout.preferredWidth: mItem.checkable ? Math.round(14 * Theme.uiScale) : 0
            }

            Text {
                id: itemLabel
                text: mItem.text
                font.pixelSize: Theme.fontSizeSmall
                font.family: Theme.fontFamily
                color: mItem.enabled ? (mItem.highlighted ? Theme.textPrimary : Theme.textSecondary) : Theme.textMuted
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            Text {
                visible: mItem.subMenu !== null && mItem.subMenu !== undefined
                text: "›"
                font.pixelSize: Math.round(13 * Theme.uiScale)
                font.bold: true
                font.family: Theme.fontFamily
                color: mItem.highlighted ? Theme.accent : Theme.textMuted
            }
        }

        background: Rectangle {
            implicitHeight: Math.round(26 * Theme.uiScale)
            color: mItem.highlighted ? Theme.selection : "transparent"
            radius: Theme.cornerRadiusSmall
        }
    }

    component MenuBarSeparator: MenuSeparator {
        id: mSep
        implicitHeight: visible ? Math.round(7 * Theme.uiScale) : 0
        topPadding: Math.round(3 * Theme.uiScale)
        bottomPadding: Math.round(3 * Theme.uiScale)
        leftPadding: Math.round(4 * Theme.uiScale)
        rightPadding: Math.round(4 * Theme.uiScale)
        contentItem: Rectangle {
            implicitHeight: 1
            color: Theme.panelBorder
            visible: mSep.visible
        }
    }

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
        width: fileMenuText.implicitWidth + Math.round(14 * Theme.uiScale)
        height: Math.round(22 * Theme.uiScale)
        radius: Math.round(3 * Theme.uiScale)
        color: (fileMenuMouse.containsMouse || fileMenu.visible) ? Theme.surfaceElevated : "transparent"
        Text {
            id: fileMenuText
            anchors.centerIn: parent
            text: "File"
            font.pixelSize: Theme.fontSizeSmall
            color: (fileMenuMouse.containsMouse || fileMenu.visible) ? Theme.textPrimary : Theme.textSecondary
        }
        MouseArea {
            id: fileMenuMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                if (fileMenu.visible) {
                    fileMenu.close();
                } else {
                    root.closeAllMenus();
                    fileMenu.open();
                }
            }
            onEntered: {
                if (root.isAnyMenuOpen && !fileMenu.visible) {
                    root.closeAllMenus();
                    fileMenu.open();
                }
            }
        }
        MenuBarPopup {
            id: fileMenu
            y: parent.height
            MenuBarItem {
                text: "Open Audio File... (Ctrl+O)"
                onTriggered: root.requestOpenAudioFile()
            }
            MenuBarItem {
                text: "Open Folder... (Ctrl+Shift+O)"
                onTriggered: root.requestOpenFolder()
            }
            MenuBarItem {
                text: "Open URL / Network Stream... (Ctrl+U)"
                onTriggered: root.requestOpenUrl()
            }
            MenuBarItem {
                text: "Open CUE Sheet..."
                onTriggered: root.requestOpenCueSheet()
            }
            MenuBarSeparator {}
            MenuBarItem {
                text: "Add Folder to Library..."
                onTriggered: root.requestAddFolderToLibrary()
            }
            MenuBarItem {
                text: "Export Active View / Playlist... (Ctrl+E)"
                onTriggered: root.requestExportView()
            }
            MenuBarSeparator {}
            MenuBarItem {
                text: "Preferences & Settings... (Ctrl+,)"
                onTriggered: root.requestPreferences(0)
            }
            MenuBarSeparator {}
            MenuBarItem {
                text: "Minimize to System Tray (Ctrl+W)"
                onTriggered: root.requestHideWindow()
            }
            MenuBarItem {
                text: "Exit (Ctrl+Q)"
                onTriggered: Qt.quit()
            }
        }
    }

    // Edit Menu Button
    Rectangle {
        width: editMenuText.implicitWidth + Math.round(14 * Theme.uiScale)
        height: Math.round(22 * Theme.uiScale)
        radius: Math.round(3 * Theme.uiScale)
        color: (editMenuMouse.containsMouse || editMenu.visible) ? Theme.surfaceElevated : "transparent"
        Text {
            id: editMenuText
            anchors.centerIn: parent
            text: "Edit"
            font.pixelSize: Theme.fontSizeSmall
            color: (editMenuMouse.containsMouse || editMenu.visible) ? Theme.textPrimary : Theme.textSecondary
        }
        MouseArea {
            id: editMenuMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                if (editMenu.visible) {
                    editMenu.close();
                } else {
                    root.closeAllMenus();
                    editMenu.open();
                }
            }
            onEntered: {
                if (root.isAnyMenuOpen && !editMenu.visible) {
                    root.closeAllMenus();
                    editMenu.open();
                }
            }
        }
        MenuBarPopup {
            id: editMenu
            y: parent.height
            MenuBarItem {
                text: bridge.undoActionName.length > 0 ? ("Undo " + bridge.undoActionName + " (Ctrl+Z)") : "Undo (Ctrl+Z)"
                enabled: bridge.canUndo
                onTriggered: bridge.undo()
            }
            MenuBarItem {
                text: bridge.redoActionName.length > 0 ? ("Redo " + bridge.redoActionName + " (Ctrl+Y)") : "Redo (Ctrl+Y)"
                enabled: bridge.canRedo
                onTriggered: bridge.redo()
            }
            MenuBarSeparator {}
            MenuBarItem {
                text: "Select All (Ctrl+A)"
                onTriggered: bridge.trackModel.selectAll()
            }
            MenuBarItem {
                text: "Invert Selection (Ctrl+Shift+A)"
                onTriggered: bridge.trackModel.invertSelection()
            }
            MenuBarItem {
                text: "Clear Selection (Esc)"
                enabled: bridge.trackModel.selectedCount > 0
                onTriggered: bridge.trackModel.clearSelection()
            }
            MenuBarSeparator {}
            MenuBarItem {
                text: "Find / Instant Search (Ctrl+F)"
                onTriggered: root.requestFocusSearch()
            }
            MenuBarItem {
                text: "Clear Search Filter (Shift+Esc)"
                onTriggered: root.requestClearSearch()
            }
            MenuBarSeparator {}
            MenuBarItem {
                text: "Edit Track Tags... (Ctrl+T)"
                enabled: bridge.trackModel.selectedCount > 0
                onTriggered: root.requestEditTags()
            }
            MenuBarItem {
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
        width: viewMenuText.implicitWidth + Math.round(14 * Theme.uiScale)
        height: Math.round(22 * Theme.uiScale)
        radius: Math.round(3 * Theme.uiScale)
        color: (viewMenuMouse.containsMouse || viewMenu.visible) ? Theme.surfaceElevated : "transparent"
        Text {
            id: viewMenuText
            anchors.centerIn: parent
            text: "View"
            font.pixelSize: Theme.fontSizeSmall
            color: (viewMenuMouse.containsMouse || viewMenu.visible) ? Theme.textPrimary : Theme.textSecondary
        }
        MouseArea {
            id: viewMenuMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                if (viewMenu.visible) {
                    viewMenu.close();
                } else {
                    root.closeAllMenus();
                    viewMenu.open();
                }
            }
            onEntered: {
                if (root.isAnyMenuOpen && !viewMenu.visible) {
                    root.closeAllMenus();
                    viewMenu.open();
                }
            }
        }
        MenuBarPopup {
            id: viewMenu
            y: parent.height
            MenuBarItem {
                text: (root.showLeftPanel ? "Hide" : "Show") + " Left Navigator (Ctrl+1)"
                onTriggered: root.showLeftPanel = !root.showLeftPanel
            }
            MenuBarItem {
                text: (root.showColumnBrowser ? "Hide" : "Show") + " Category Filters (Ctrl+2)"
                onTriggered: root.showColumnBrowser = !root.showColumnBrowser
            }
            MenuBarItem {
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
            MenuBarItem {
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
            MenuBarItem {
                text: (root.showEqualizer ? "Hide" : "Show") + " Parametric Equalizer (Ctrl+5)"
                onTriggered: root.requestToggleEqualizer()
            }
            MenuBarItem {
                text: (root.showStatusBar ? "Hide" : "Show") + " Bottom Status Bar"
                checkable: true
                checked: root.showStatusBar
                onTriggered: root.showStatusBar = !root.showStatusBar
            }
            MenuBarSeparator {}
            MenuBarPopup {
                title: "View Modes (Active Tab)"
                MenuBarItem {
                    text: "Tracks Table"
                    checkable: true
                    checked: bridge.tabModel && (bridge.tabModel.activeViewId === "tracks_default" || (bridge.viewRegistry && bridge.viewRegistry.getBaseRendererType(bridge.tabModel.activeViewId) === "tracks"))
                    onTriggered: { if (bridge.tabModel) bridge.tabModel.updateActiveView("tracks_default") }
                }
                MenuBarItem {
                    text: "Album Covers Grid"
                    checkable: true
                    checked: bridge.tabModel && (bridge.tabModel.activeViewId === "covers_default" || (bridge.viewRegistry && bridge.viewRegistry.getBaseRendererType(bridge.tabModel.activeViewId) === "covers"))
                    onTriggered: { if (bridge.tabModel) bridge.tabModel.updateActiveView("covers_default") }
                }
                MenuBarItem {
                    text: "Album & Tracks"
                    checkable: true
                    checked: bridge.tabModel && (bridge.tabModel.activeViewId === "album_and_tracks_default" || (bridge.viewRegistry && bridge.viewRegistry.getBaseRendererType(bridge.tabModel.activeViewId) === "album_and_tracks"))
                    onTriggered: { if (bridge.tabModel) bridge.tabModel.updateActiveView("album_and_tracks_default") }
                }
                MenuBarItem {
                    text: "Artists Grid"
                    checkable: true
                    checked: bridge.tabModel && (bridge.tabModel.activeViewId === "artists_default" || (bridge.viewRegistry && bridge.viewRegistry.getBaseRendererType(bridge.tabModel.activeViewId) === "artists"))
                    onTriggered: { if (bridge.tabModel) bridge.tabModel.updateActiveView("artists_default") }
                }
                MenuBarItem {
                    text: "Now Playing View"
                    checkable: true
                    checked: bridge.tabModel && (bridge.tabModel.activeViewId === "now_playing_default" || (bridge.viewRegistry && (bridge.viewRegistry.getBaseRendererType(bridge.tabModel.activeViewId) === "now_playing" || bridge.viewRegistry.getBaseRendererType(bridge.tabModel.activeViewId) === "jukebox")))
                    onTriggered: { if (bridge.tabModel) bridge.tabModel.updateActiveView("now_playing_default") }
                }
                MenuBarItem {
                    text: "Audio Visualizer Window... (Ctrl+Shift+V)"
                    onTriggered: root.requestVisualizerDialog()
                }
                MenuBarItem {
                    text: "Listening History"
                    checkable: true
                    checked: bridge.tabModel && (bridge.tabModel.activeViewId === "history_default" || (bridge.viewRegistry && bridge.viewRegistry.getBaseRendererType(bridge.tabModel.activeViewId) === "history"))
                    onTriggered: { if (bridge.tabModel) bridge.tabModel.updateActiveView("history_default") }
                }
                MenuBarSeparator {}
                MenuBarItem {
                    text: "Group Albums with Guest/Featuring Artists"
                    checkable: true
                    checked: bridge.groupAlbumsByTitle
                    onTriggered: bridge.setGroupAlbumsByTitle(!bridge.groupAlbumsByTitle)
                }
            }
            MenuBarSeparator {}
            MenuBarPopup {
                title: "Layout Presets"
                MenuBarItem {
                    text: "Studio (Pro Workspace)"
                    checkable: true
                    checked: bridge.layoutModel && bridge.layoutModel.currentPreset === "studio"
                    onTriggered: bridge.applyLayoutPreset("studio")
                }
                MenuBarItem {
                    text: "Showcase (Art Gallery)"
                    checkable: true
                    checked: bridge.layoutModel && bridge.layoutModel.currentPreset === "showcase"
                    onTriggered: bridge.applyLayoutPreset("showcase")
                }
                MenuBarItem {
                    text: "Catalog (Multi-Pane Explorer)"
                    checkable: true
                    checked: bridge.layoutModel && bridge.layoutModel.currentPreset === "catalog"
                    onTriggered: bridge.applyLayoutPreset("catalog")
                }
                MenuBarItem {
                    text: "Stage (Immersive Player)"
                    checkable: true
                    checked: bridge.layoutModel && bridge.layoutModel.currentPreset === "stage"
                    onTriggered: bridge.applyLayoutPreset("stage")
                }
                MenuBarItem {
                    text: "Streamlined (Minimal)"
                    checkable: true
                    checked: bridge.layoutModel && bridge.layoutModel.currentPreset === "compact"
                    onTriggered: bridge.applyLayoutPreset("compact")
                }
            }
            MenuBarItem {
                text: "Arrange Panels & Docking Zones..."
                onTriggered: root.requestArrangePanels()
            }
            MenuBarItem {
                text: (root.isMiniPlayer ? "Restore Full Player" : "Mini-Player Mode") + " (Ctrl+Shift+M / F10)"
                onTriggered: root.requestToggleMiniPlayer()
            }
            MenuBarItem {
                text: (root.isFullscreen ? "Exit Fullscreen" : "Fullscreen") + " (F11)"
                onTriggered: root.requestToggleFullscreen()
            }
            MenuBarSeparator {}
            MenuBarPopup {
                title: "Zoom UI"
                MenuBarItem {
                    text: "Zoom In (Ctrl++)"
                    onTriggered: root.zoomIn()
                }
                MenuBarItem {
                    text: "Zoom Out (Ctrl+-)"
                    onTriggered: root.zoomOut()
                }
                MenuBarItem {
                    text: "Reset Zoom (100%) (Ctrl+0)"
                    onTriggered: root.zoomReset()
                }
                MenuBarSeparator {}
                MenuBarItem {
                    text: "75%"
                    checkable: true
                    checked: Math.abs(Theme.uiScale - 0.75) < 0.04
                    onTriggered: Theme.uiScale = 0.75
                }
                MenuBarItem {
                    text: "90%"
                    checkable: true
                    checked: Math.abs(Theme.uiScale - 0.9) < 0.04
                    onTriggered: Theme.uiScale = 0.9
                }
                MenuBarItem {
                    text: "100% (Standard)"
                    checkable: true
                    checked: Math.abs(Theme.uiScale - 1.0) < 0.04
                    onTriggered: Theme.uiScale = 1.0
                }
                MenuBarItem {
                    text: "110%"
                    checkable: true
                    checked: Math.abs(Theme.uiScale - 1.1) < 0.04
                    onTriggered: Theme.uiScale = 1.1
                }
                MenuBarItem {
                    text: "125%"
                    checkable: true
                    checked: Math.abs(Theme.uiScale - 1.25) < 0.04
                    onTriggered: Theme.uiScale = 1.25
                }
                MenuBarItem {
                    text: "150%"
                    checkable: true
                    checked: Math.abs(Theme.uiScale - 1.5) < 0.04
                    onTriggered: Theme.uiScale = 1.5
                }
            }
            MenuBarPopup {
                id: themesQuickMenu
                title: "Themes Quick Switch"
                Instantiator {
                    model: Theme.availableThemes
                    onObjectAdded: (index, object) => themesQuickMenu.insertItem(index, object)
                    onObjectRemoved: (index, object) => themesQuickMenu.removeItem(object)
                    delegate: MenuBarItem {
                        text: (modelData.name || modelData.id) + (Theme.themeId === modelData.id ? "  ✓" : "")
                        onTriggered: Theme.themeId = modelData.id
                    }
                }
                MenuBarSeparator {}
                MenuBarItem {
                    text: "Manage Themes..."
                    onTriggered: root.requestPreferences(1)
                }
            }
        }
    }

    // Tabs Menu Button
    Rectangle {
        width: tabsMenuText.implicitWidth + Math.round(14 * Theme.uiScale)
        height: Math.round(22 * Theme.uiScale)
        radius: Math.round(3 * Theme.uiScale)
        color: (tabsMenuMouse.containsMouse || tabsMenu.visible) ? Theme.surfaceElevated : "transparent"
        Text {
            id: tabsMenuText
            anchors.centerIn: parent
            text: "Tabs"
            font.pixelSize: Theme.fontSizeSmall
            color: (tabsMenuMouse.containsMouse || tabsMenu.visible) ? Theme.textPrimary : Theme.textSecondary
        }
        MouseArea {
            id: tabsMenuMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                if (tabsMenu.visible) {
                    tabsMenu.close();
                } else {
                    root.closeAllMenus();
                    tabsMenu.open();
                }
            }
            onEntered: {
                if (root.isAnyMenuOpen && !tabsMenu.visible) {
                    root.closeAllMenus();
                    tabsMenu.open();
                }
            }
        }
        MenuBarPopup {
            id: tabsMenu
            y: parent.height
            MenuBarItem {
                text: "New Tab (Ctrl+T)"
                onTriggered: { if (bridge.tabModel) bridge.tabModel.openTab("library://music", "Music") }
            }
            MenuBarItem {
                text: "Close Tab (Ctrl+W)"
                enabled: bridge.tabModel && bridge.tabModel.tabCount > 1
                onTriggered: { if (bridge.tabModel) bridge.tabModel.closeTab(bridge.tabModel.activeTabIndex, false) }
            }
            MenuBarItem {
                text: "Close Other Tabs"
                enabled: bridge.tabModel && bridge.tabModel.tabCount > 1
                onTriggered: { if (bridge.tabModel) bridge.tabModel.closeOtherTabs(bridge.tabModel.activeTabIndex) }
            }
            MenuBarItem {
                text: "Reopen Closed Tab (Ctrl+Shift+T)"
                enabled: bridge.tabModel && bridge.tabModel.canReopenClosedTab
                onTriggered: { if (bridge.tabModel) bridge.tabModel.reopenLastClosedTab() }
            }
            MenuBarItem {
                text: "Duplicate Tab"
                onTriggered: { if (bridge.tabModel) bridge.tabModel.duplicateTab(bridge.tabModel.activeTabIndex) }
            }
            MenuBarSeparator {}
            MenuBarItem {
                text: (bridge.tabModel && bridge.tabModel.activeLockMode === 0) ? "Lock Tab" : "Unlock Tab"
                onTriggered: {
                    if (bridge.tabModel) {
                        if (bridge.tabModel.activeLockMode === 0) {
                            bridge.tabModel.setTabLockMode(bridge.tabModel.activeTabIndex, 1)
                        } else {
                            bridge.tabModel.unlockTab(bridge.tabModel.activeTabIndex)
                        }
                    }
                }
            }
            MenuBarItem {
                text: (bridge.tabModel && bridge.tabModel.activeIsPinned) ? "Unpin Tab" : "Pin Tab"
                onTriggered: {
                    if (bridge.tabModel) {
                        bridge.tabModel.setTabPinned(bridge.tabModel.activeTabIndex, !bridge.tabModel.activeIsPinned)
                    }
                }
            }
            MenuBarItem {
                text: "Set as Root Lock (Lock to Current Location)"
                onTriggered: {
                    if (bridge.tabModel) {
                        bridge.tabModel.setTabRootLock(bridge.tabModel.activeTabIndex, bridge.tabModel.activeSourceId)
                    }
                }
            }
        }
    }

    // Playback Menu Button
    Rectangle {
        width: playbackMenuText.implicitWidth + Math.round(14 * Theme.uiScale)
        height: Math.round(22 * Theme.uiScale)
        radius: Math.round(3 * Theme.uiScale)
        color: (playbackMenuMouse.containsMouse || playbackMenu.visible) ? Theme.surfaceElevated : "transparent"
        Text {
            id: playbackMenuText
            anchors.centerIn: parent
            text: "Playback"
            font.pixelSize: Theme.fontSizeSmall
            color: (playbackMenuMouse.containsMouse || playbackMenu.visible) ? Theme.textPrimary : Theme.textSecondary
        }
        MouseArea {
            id: playbackMenuMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                if (playbackMenu.visible) {
                    playbackMenu.close();
                } else {
                    root.closeAllMenus();
                    playbackMenu.open();
                }
            }
            onEntered: {
                if (root.isAnyMenuOpen && !playbackMenu.visible) {
                    root.closeAllMenus();
                    playbackMenu.open();
                }
            }
        }
        MenuBarPopup {
            id: playbackMenu
            y: parent.height
            MenuBarItem {
                text: bridge.isPlaying ? "Pause (Space)" : "Play (Space)"
                onTriggered: bridge.togglePlayPause()
            }
            MenuBarItem {
                text: "Stop (Ctrl+.)"
                onTriggered: bridge.stop()
            }
            MenuBarItem {
                text: "Stop After Current Track (Shift+Space)"
                checkable: true
                checked: bridge.stopAfterCurrentTrack
                onTriggered: bridge.toggleStopAfterCurrentTrack()
            }
            MenuBarItem {
                text: "Next Track (Ctrl+Right)"
                onTriggered: bridge.nextTrack()
            }
            MenuBarItem {
                text: "Previous Track (Ctrl+Left)"
                onTriggered: bridge.previousTrack()
            }
            MenuBarSeparator {}
            MenuBarPopup {
                title: "Seeking"
                MenuBarItem {
                    text: "Short Seek Backward 5s (Left)"
                    onTriggered: bridge.seek(bridge.positionMs - 5000)
                }
                MenuBarItem {
                    text: "Short Seek Forward 5s (Right)"
                    onTriggered: bridge.seek(bridge.positionMs + 5000)
                }
                MenuBarItem {
                    text: "Long Seek Backward 30s (Shift+Left)"
                    onTriggered: bridge.seek(bridge.positionMs - 30000)
                }
                MenuBarItem {
                    text: "Long Seek Forward 30s (Shift+Right)"
                    onTriggered: bridge.seek(bridge.positionMs + 30000)
                }
                MenuBarSeparator {}
                MenuBarItem {
                    text: "Seek to Track Beginning (Home)"
                    onTriggered: bridge.seek(0)
                }
                MenuBarItem {
                    text: "Seek to Track End (End)"
                    onTriggered: bridge.seek(bridge.durationMs)
                }
            }
            MenuBarSeparator {}
            MenuBarItem {
                text: "Volume Up 5% (Ctrl+Up)"
                onTriggered: bridge.setVolume(bridge.volume + 0.05)
            }
            MenuBarItem {
                text: "Volume Down 5% (Ctrl+Down)"
                onTriggered: bridge.setVolume(bridge.volume - 0.05)
            }
            MenuBarItem {
                text: "Mute Toggle (Ctrl+M)"
                checkable: true
                checked: bridge.isMuted
                onTriggered: bridge.toggleMute()
            }
            MenuBarSeparator {}
            MenuBarPopup {
                title: "Playback Speed"
                MenuBarItem {
                    text: "0.5x"
                    checkable: true
                    checked: Math.abs(bridge.playbackRate - 0.5) < 0.05
                    onTriggered: bridge.setPlaybackRate(0.5)
                }
                MenuBarItem {
                    text: "0.75x"
                    checkable: true
                    checked: Math.abs(bridge.playbackRate - 0.75) < 0.05
                    onTriggered: bridge.setPlaybackRate(0.75)
                }
                MenuBarItem {
                    text: "1.0x (Normal)"
                    checkable: true
                    checked: Math.abs(bridge.playbackRate - 1.0) < 0.05
                    onTriggered: bridge.setPlaybackRate(1.0)
                }
                MenuBarItem {
                    text: "1.25x"
                    checkable: true
                    checked: Math.abs(bridge.playbackRate - 1.25) < 0.05
                    onTriggered: bridge.setPlaybackRate(1.25)
                }
                MenuBarItem {
                    text: "1.5x"
                    checkable: true
                    checked: Math.abs(bridge.playbackRate - 1.5) < 0.05
                    onTriggered: bridge.setPlaybackRate(1.5)
                }
                MenuBarItem {
                    text: "2.0x"
                    checkable: true
                    checked: Math.abs(bridge.playbackRate - 2.0) < 0.05
                    onTriggered: bridge.setPlaybackRate(2.0)
                }
            }
            MenuBarPopup {
                title: "A-B Looping"
                MenuBarItem {
                    text: "Set Loop Point A (" + (bridge.loopPointA >= 0 ? root.formatTimestamp(bridge.loopPointA) : "--:--") + ")  ([)"
                    onTriggered: bridge.setLoopPointA()
                }
                MenuBarItem {
                    text: "Set Loop Point B (" + (bridge.loopPointB >= 0 ? root.formatTimestamp(bridge.loopPointB) : "--:--") + ")  (])"
                    onTriggered: bridge.setLoopPointB()
                }
                MenuBarItem {
                    text: "Clear Loop (\\)"
                    enabled: bridge.isLoopActive || bridge.loopPointA >= 0 || bridge.loopPointB >= 0
                    onTriggered: bridge.clearLoop()
                }
            }
            MenuBarSeparator {}
            MenuBarItem {
                text: "Cycle Shuffle Mode (Ctrl+S)"
                onTriggered: bridge.cycleShuffleMode()
            }
            MenuBarItem {
                text: "Cycle Repeat Mode (Ctrl+R)"
                onTriggered: bridge.cycleRepeatMode()
            }
            MenuBarSeparator {}
            MenuBarItem {
                text: "Play All Tracks"
                onTriggered: bridge.playAll()
            }
            MenuBarItem {
                text: "Shuffle All Tracks"
                onTriggered: bridge.shuffleAll()
            }
        }
    }

    // Library Menu Button
    Rectangle {
        width: libraryMenuText.implicitWidth + Math.round(14 * Theme.uiScale)
        height: Math.round(22 * Theme.uiScale)
        radius: Math.round(3 * Theme.uiScale)
        color: (libraryMenuMouse.containsMouse || libraryMenu.visible) ? Theme.surfaceElevated : "transparent"
        Text {
            id: libraryMenuText
            anchors.centerIn: parent
            text: "Library"
            font.pixelSize: Theme.fontSizeSmall
            color: (libraryMenuMouse.containsMouse || libraryMenu.visible) ? Theme.textPrimary : Theme.textSecondary
        }
        MouseArea {
            id: libraryMenuMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                if (libraryMenu.visible) {
                    libraryMenu.close();
                } else {
                    root.closeAllMenus();
                    libraryMenu.open();
                }
            }
            onEntered: {
                if (root.isAnyMenuOpen && !libraryMenu.visible) {
                    root.closeAllMenus();
                    libraryMenu.open();
                }
            }
        }
        MenuBarPopup {
            id: libraryMenu
            y: parent.height
            MenuBarItem {
                text: "Rescan All Monitored Folders (F5)"
                onTriggered: bridge.rescanAllMonitoredFolders()
            }
            MenuBarItem {
                text: "Quick Incremental Scan (Ctrl+F5)"
                onTriggered: bridge.incrementalQuickScan()
            }
            MenuBarItem {
                text: "Manage Monitored Folders..."
                onTriggered: root.requestPreferences(3)
            }
            MenuBarItem {
                text: "Group Albums with Guest/Featuring Artists"
                checkable: true
                checked: bridge.groupAlbumsByTitle
                onTriggered: bridge.setGroupAlbumsByTitle(!bridge.groupAlbumsByTitle)
            }
            MenuBarSeparator {}
            MenuBarItem {
                text: "Create New Playlist... (Ctrl+N)"
                onTriggered: root.requestNewPlaylist(false)
            }
            MenuBarItem {
                text: "Create Smart Playlist... (Ctrl+Shift+N)"
                onTriggered: root.requestNewPlaylist(true)
            }
            MenuBarItem {
                text: "Import Playlist..."
                onTriggered: root.requestImportPlaylist()
            }
            MenuBarItem {
                text: "Export Active View / Playlist... (Ctrl+E)"
                onTriggered: root.requestExportView()
            }
            MenuBarSeparator {}
            MenuBarItem {
                text: "Deduplicate Library Tracks"
                onTriggered: bridge.deduplicateTracks()
            }
            MenuBarItem {
                text: "Check for Dead / Broken Files"
                onTriggered: bridge.purgeMissingTracks()
            }
            MenuBarItem {
                text: "Clear Cover Art Cache"
                onTriggered: bridge.clearCoverArtCache()
            }
            MenuBarSeparator {}
            MenuBarItem {
                text: "Clear Entire Library..."
                onTriggered: bridge.clearLibrary()
            }
        }
    }

    // Tools Menu Button
    Rectangle {
        width: toolsMenuText.implicitWidth + Math.round(14 * Theme.uiScale)
        height: Math.round(22 * Theme.uiScale)
        radius: Math.round(3 * Theme.uiScale)
        color: (toolsMenuMouse.containsMouse || toolsMenu.visible) ? Theme.surfaceElevated : "transparent"
        Text {
            id: toolsMenuText
            anchors.centerIn: parent
            text: "Tools"
            font.pixelSize: Theme.fontSizeSmall
            color: (toolsMenuMouse.containsMouse || toolsMenu.visible) ? Theme.textPrimary : Theme.textSecondary
        }
        MouseArea {
            id: toolsMenuMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                if (toolsMenu.visible) {
                    toolsMenu.close();
                } else {
                    root.closeAllMenus();
                    toolsMenu.open();
                }
            }
            onEntered: {
                if (root.isAnyMenuOpen && !toolsMenu.visible) {
                    root.closeAllMenus();
                    toolsMenu.open();
                }
            }
        }
        MenuBarPopup {
            id: toolsMenu
            y: parent.height
            MenuBarItem {
                text: "Metadata Tag Editor... (Ctrl+T)"
                onTriggered: root.requestEditTags()
            }
            MenuBarItem {
                text: (root.showEqualizer ? "Hide" : "Show") + " Parametric Equalizer (Ctrl+Shift+E)"
                onTriggered: root.requestToggleEqualizer()
            }
            MenuBarItem {
                text: "Playback Sleep Timer..."
                onTriggered: root.requestSleepTimer()
            }
            MenuBarItem {
                text: "Library Health & Missing Files Inspector..."
                onTriggered: root.requestLibraryHealth()
            }
            MenuBarSeparator {}
            MenuBarPopup {
                id: audioDeviceMenu
                title: "Audio Output Device"
                Instantiator {
                    model: bridge.availableAudioDevices
                    onObjectAdded: (index, object) => audioDeviceMenu.insertItem(index, object)
                    onObjectRemoved: (index, object) => audioDeviceMenu.removeItem(object)
                    delegate: MenuBarItem {
                        text: modelData + (bridge.currentAudioDevice === modelData ? "  ✓" : "")
                        checkable: true
                        checked: bridge.currentAudioDevice === modelData
                        onTriggered: bridge.setCurrentAudioDevice(modelData)
                    }
                }
            }
            MenuBarSeparator {}
            MenuBarItem {
                text: "Preferences & Settings... (Ctrl+,)"
                onTriggered: root.requestPreferences(0)
            }
            MenuBarItem {
                text: "Manage Plugins & Extensions..."
                onTriggered: root.requestPreferences(5)
            }
        }
    }

    // Help Menu Button
    Rectangle {
        width: helpMenuText.implicitWidth + Math.round(14 * Theme.uiScale)
        height: Math.round(22 * Theme.uiScale)
        radius: Math.round(3 * Theme.uiScale)
        color: (helpMenuMouse.containsMouse || helpMenu.visible) ? Theme.surfaceElevated : "transparent"
        Text {
            id: helpMenuText
            anchors.centerIn: parent
            text: "Help"
            font.pixelSize: Theme.fontSizeSmall
            color: (helpMenuMouse.containsMouse || helpMenu.visible) ? Theme.textPrimary : Theme.textSecondary
        }
        MouseArea {
            id: helpMenuMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                if (helpMenu.visible) {
                    helpMenu.close();
                } else {
                    root.closeAllMenus();
                    helpMenu.open();
                }
            }
            onEntered: {
                if (root.isAnyMenuOpen && !helpMenu.visible) {
                    root.closeAllMenus();
                    helpMenu.open();
                }
            }
        }
        MenuBarPopup {
            id: helpMenu
            y: parent.height
            MenuBarItem {
                text: "Documentation & User Guide (F1)"
                onTriggered: Qt.openUrlExternally("https://github.com/ShamalLakshan/PicAudio")
            }
            MenuBarItem {
                text: "Keyboard Shortcuts Cheat Sheet (Ctrl+/)"
                onTriggered: root.requestShortcuts()
            }
            MenuBarItem {
                text: "Audio Pipeline Diagnostics... (Ctrl+J)"
                onTriggered: root.requestDiagnostics()
            }
            MenuBarItem {
                text: "Open Application Log File"
                onTriggered: Qt.openUrlExternally("file://" + bridge.getLogFilePath())
            }
            MenuBarSeparator {}
            MenuBarItem {
                text: "Report an Issue / GitHub..."
                onTriggered: Qt.openUrlExternally("https://github.com/ShamalLakshan/PicAudio/issues")
            }
            MenuBarSeparator {}
            MenuBarItem {
                text: "About PicAudio"
                onTriggered: root.requestAbout()
            }
        }
    }
}
