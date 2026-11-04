import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

Rectangle {
    id: root

    property int activeNavSection: 0
    property string activePlaylistName: ""
    property int mainViewMode: 1

    signal navSectionSelected(int nav, int mode)
    signal playlistSelected(string name)
    signal requestSwitchToNowPlaying()
    signal requestShowColumnBrowser(bool show)
    signal requestAddMusicFolder()
    signal requestFolderContextMenu(string folderPath)
    signal requestPlaylistContextMenu(string playlistName, bool isSmart)
    signal requestExplorerContextMenu()

    Layout.preferredWidth: bridge.leftPanelWidth
    Layout.fillHeight: true
    color: Theme.surface

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        onClicked: (mouse) => {
            if (mouse.button === Qt.RightButton) {
                root.requestExplorerContextMenu();
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Explorer Header
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 28
            color: Theme.surfaceElevated
            border.color: Theme.panelBorder
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                Text {
                    text: "LIBRARY EXPLORER"
                    font.pixelSize: Theme.fontSizeSmall - 1
                    font.bold: true
                    font.letterSpacing: 1.2
                    color: Theme.textMuted
                }
                Item { Layout.fillWidth: true }
                Text {
                    text: bridge.totalTracks + " items"
                    font.pixelSize: Theme.fontSizeSmall - 1
                    color: Theme.textMuted
                }
            }
        }

        // Navigation Tree Scroll
        ScrollView {
            id: sidebarScrollView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ScrollBar.vertical.policy: ScrollBar.AsNeeded
            contentWidth: availableWidth

            ColumnLayout {
                width: sidebarScrollView.availableWidth
                spacing: 1

                // Section: LIBRARY
                Item { height: 6; width: 1 }
                Text {
                    text: "  LIBRARY"
                    font.pixelSize: Theme.fontSizeSmall - 2
                    font.bold: true
                    font.letterSpacing: 1.0
                    color: Theme.textMuted
                    Layout.leftMargin: 8
                }

                Repeater {
                    model: [
                        { title: "All Music Tracks", count: bridge.totalTracks, icon: "table", nav: 0, mode: 1 },
                        { title: "Albums", count: bridge.totalAlbums, icon: "grid", nav: 1, mode: 0 },
                        { title: "Artists", count: bridge.artistsList.length, icon: "disc", nav: 2, mode: 1 },
                        { title: "Genres", count: bridge.genresList.length, icon: "music", nav: 3, mode: 1 }
                    ]

                    delegate: Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 26
                        color: root.activeNavSection === modelData.nav ? Theme.selection : (navItemMouse.containsMouse ? Theme.surfaceElevated : "transparent")

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 16
                            anchors.rightMargin: 10
                            spacing: 8

                            VectorIcon {
                                name: modelData.icon
                                width: 11
                                height: 11
                                color: root.activeNavSection === modelData.nav ? Theme.accent : Theme.textMuted
                            }

                            Text {
                                text: modelData.title
                                font.pixelSize: Theme.fontSizeSmall
                                color: root.activeNavSection === modelData.nav ? Theme.textPrimary : Theme.textSecondary
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }

                            Text {
                                text: "" + modelData.count
                                font.pixelSize: Theme.fontSizeSmall - 1
                                color: root.activeNavSection === modelData.nav ? Theme.textPrimary : Theme.textMuted
                            }
                        }

                        MouseArea {
                            id: navItemMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                root.activeNavSection = modelData.nav;
                                root.navSectionSelected(modelData.nav, modelData.mode);
                                if (modelData.nav === 2 || modelData.nav === 3) {
                                    root.requestShowColumnBrowser(true);
                                } else {
                                    bridge.resetFilters();
                                }
                            }
                        }
                    }
                }

                // Section: PLAYLISTS
                Item { height: 10; width: 1 }
                Text {
                    text: "  PLAYLISTS & QUEUE"
                    font.pixelSize: Theme.fontSizeSmall - 2
                    font.bold: true
                    font.letterSpacing: 1.0
                    color: Theme.textMuted
                    Layout.leftMargin: 8
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 26
                    color: (root.activeNavSection === 4 && root.mainViewMode === 4) ? Theme.selection : (npMouse.containsMouse ? Theme.surfaceElevated : "transparent")

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 16
                        anchors.rightMargin: 10
                        spacing: 8
                        VectorIcon { name: "disc"; width: 11; height: 11; color: (root.activeNavSection === 4 && root.mainViewMode === 4) ? Theme.accent : Theme.textMuted }
                        Text { text: "Now Playing"; font.pixelSize: Theme.fontSizeSmall; color: (root.activeNavSection === 4 && root.mainViewMode === 4) ? Theme.textPrimary : Theme.textSecondary; Layout.fillWidth: true }
                    }
                    MouseArea {
                        id: npMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            root.requestSwitchToNowPlaying();
                        }
                    }
                }

                Repeater {
                    model: bridge.availablePlaylists
                    delegate: Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 26
                        color: plItemDropArea.containsDrag ? Theme.selection : ((root.activeNavSection === 5 && root.activePlaylistName === modelData) ? Theme.selection : (plItemMouse.containsMouse ? Theme.surfaceElevated : "transparent"))
                        border.color: plItemDropArea.containsDrag ? Theme.accent : "transparent"
                        border.width: plItemDropArea.containsDrag ? 1 : 0
                        radius: Theme.cornerRadiusSmall

                        DropArea {
                            id: plItemDropArea
                            anchors.fill: parent
                            onEntered: (drag) => {
                                if (drag.hasUrls) drag.acceptProposedAction();
                            }
                            onDropped: (drop) => {
                                if (drop.hasUrls) {
                                    bridge.handleDropOnPlaylist(drop.urls, modelData);
                                    drop.acceptProposedAction();
                                }
                            }
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 16
                            anchors.rightMargin: 10
                            spacing: 8
                            VectorIcon {
                                name: bridge.isSmartPlaylist(modelData) ? "settings" : "playlist"
                                width: 11
                                height: 11
                                color: (root.activeNavSection === 5 && root.activePlaylistName === modelData) ? Theme.accent : Theme.textMuted
                            }
                            Text {
                                text: modelData
                                font.pixelSize: Theme.fontSizeSmall
                                color: (root.activeNavSection === 5 && root.activePlaylistName === modelData) ? Theme.textPrimary : Theme.textSecondary
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                        }
                        MouseArea {
                            id: plItemMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            acceptedButtons: Qt.LeftButton | Qt.RightButton
                            onClicked: (mouse) => {
                                if (mouse.button === Qt.RightButton) {
                                    root.requestPlaylistContextMenu(modelData, bridge.isSmartPlaylist(modelData));
                                } else {
                                    root.activeNavSection = 5;
                                    root.playlistSelected(modelData);
                                    bridge.openPlaylist(modelData);
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 28
                    visible: plNewDropArea.containsDrag || bridge.availablePlaylists.length === 0
                    color: plNewDropArea.containsDrag ? Theme.surfaceElevated : "transparent"
                    border.color: plNewDropArea.containsDrag ? Theme.accent : Theme.panelBorder
                    border.width: 1
                    radius: Theme.cornerRadiusSmall
                    Layout.leftMargin: 8
                    Layout.rightMargin: 8

                    DropArea {
                        id: plNewDropArea
                        anchors.fill: parent
                        onEntered: (drag) => {
                            if (drag.hasUrls) drag.acceptProposedAction();
                        }
                        onDropped: (drop) => {
                            if (drop.hasUrls) {
                                bridge.handleDropOnPlaylist(drop.urls, "");
                                drop.acceptProposedAction();
                            }
                        }
                    }

                    RowLayout {
                        anchors.centerIn: parent
                        spacing: 6
                        VectorIcon { name: "plus"; width: 10; height: 10; color: Theme.accent }
                        Text {
                            text: "Drop to create playlist"
                            font.pixelSize: Theme.fontSizeSmall - 1
                            color: Theme.accent
                        }
                    }
                }

                // Section: SOURCES
                Item { height: 10; width: 1 }
                Text {
                    text: "  SOURCES"
                    font.pixelSize: Theme.fontSizeSmall - 2
                    font.bold: true
                    font.letterSpacing: 1.0
                    color: Theme.textMuted
                    Layout.leftMargin: 8
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 26
                    color: folderDropArea.containsDrag ? Theme.selection : (scanMouse2.containsMouse ? Theme.surfaceElevated : "transparent")
                    border.color: folderDropArea.containsDrag ? Theme.accent : "transparent"
                    border.width: folderDropArea.containsDrag ? 1 : 0
                    radius: Theme.cornerRadiusSmall

                    DropArea {
                        id: folderDropArea
                        anchors.fill: parent
                        onEntered: (drag) => {
                            if (drag.hasUrls) drag.acceptProposedAction();
                        }
                        onDropped: (drop) => {
                            if (drop.hasUrls) {
                                bridge.handleDropOnExplorer(drop.urls);
                                drop.acceptProposedAction();
                            }
                        }
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 16
                        anchors.rightMargin: 10
                        spacing: 8
                        VectorIcon { name: "folder"; width: 11; height: 11; color: Theme.accent }
                        Text { text: "Add Music Folder..."; font.pixelSize: Theme.fontSizeSmall; color: Theme.accentHover; Layout.fillWidth: true }
                    }
                    MouseArea {
                        id: scanMouse2
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.requestAddMusicFolder()
                    }
                }

                Repeater {
                    model: bridge.monitoredFolders
                    delegate: Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 26
                        color: folderItemMouse.containsMouse ? Theme.surfaceElevated : "transparent"

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 16
                            anchors.rightMargin: 10
                            spacing: 8

                            VectorIcon {
                                name: "folder"
                                width: 11
                                height: 11
                                color: Theme.textMuted
                            }

                            Text {
                                text: {
                                    var parts = modelData.split("/");
                                    return (parts.length > 0 && parts[parts.length - 1].length > 0) ? parts[parts.length - 1] : modelData;
                                }
                                font.pixelSize: Theme.fontSizeSmall
                                color: Theme.textSecondary
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                        }

                        MouseArea {
                            id: folderItemMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            acceptedButtons: Qt.LeftButton | Qt.RightButton
                            onClicked: (mouse) => {
                                if (mouse.button === Qt.RightButton) {
                                    root.requestFolderContextMenu(modelData);
                                } else {
                                    bridge.openFolder(modelData, false);
                                }
                            }
                            ToolTip.visible: folderItemMouse.containsMouse
                            ToolTip.text: modelData + "\nRight-click for options (Rescan, Reveal, Remove)"
                        }
                    }
                }

                Item { Layout.fillHeight: true }
            }
        }
    }
}
