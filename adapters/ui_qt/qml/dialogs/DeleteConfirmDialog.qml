import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../components"

Dialog {
    id: root
    modal: true
    focus: true
    x: Math.round(((Overlay.overlay ? Overlay.overlay.width : (parent ? parent.width : 500)) - width) / 2)
    y: Math.round(((Overlay.overlay ? Overlay.overlay.height : (parent ? parent.height : 400)) - height) / 2)
    width: Math.min(480, Overlay.overlay ? Overlay.overlay.width - 32 : 480)
    padding: 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property string deleteMode: "files" // "files", "playlist", "database"
    property var tracksToDelete: []
    property string playlistToDelete: ""

    signal playlistDeleted(string playlistName)
    signal databasePurged()

    function confirmDeleteFiles(tracks) {
        deleteMode = "files";
        tracksToDelete = tracks || [];
        open();
    }

    function confirmDeletePlaylist(name) {
        deleteMode = "playlist";
        playlistToDelete = name || "";
        open();
    }

    function confirmPurgeDatabase() {
        deleteMode = "database";
        open();
    }

    onAboutToShow: {
        var ovW = Overlay.overlay ? Overlay.overlay.width : (parent ? parent.width : 500);
        var ovH = Overlay.overlay ? Overlay.overlay.height : (parent ? parent.height : 400);
        if (x <= 0 || y <= 0 || x > ovW - 60 || y > ovH - 60) {
            x = Math.max(16, Math.round((ovW - width) / 2));
            y = Math.max(16, Math.round((ovH - height) / 2));
        }
    }

    background: Rectangle {
        color: Theme.surface
        border.color: Theme.error
        border.width: 1
        radius: Theme.cornerRadiusMedium
    }

    contentItem: Item {
        clip: true

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // Danger Dialog Header
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 44
                color: Theme.surfaceElevated
                border.color: Theme.panelBorder
                border.width: 1

                MovableHeaderArea {
                    target: root
                }

                RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.panelPadding
                anchors.rightMargin: Theme.panelPadding
                spacing: Theme.spacingMedium

                VectorIcon {
                    name: "trash"
                    width: 15
                    height: 15
                    color: Theme.error
                    Layout.alignment: Qt.AlignVCenter
                }

                Text {
                    text: {
                        if (root.deleteMode === "playlist") {
                            return "Delete Playlist: \"" + root.playlistToDelete + "\"?";
                        }
                        if (root.deleteMode === "database") {
                            return "Purge Library Database?";
                        }
                        return root.tracksToDelete.length > 1
                            ? ("Delete " + root.tracksToDelete.length + " Files from Disk?")
                            : "Delete Audio File from Disk?";
                    }
                    font.pixelSize: Theme.fontSizeLarge
                    font.bold: true
                    color: Theme.textPrimary
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }

                Rectangle {
                    width: 24
                    height: 24
                    radius: Theme.cornerRadiusSmall
                    color: closeMouse.containsMouse ? Theme.selection : "transparent"

                    VectorIcon {
                        anchors.centerIn: parent
                        name: "clear"
                        width: 10
                        height: 10
                        color: closeMouse.containsMouse ? Theme.textPrimary : Theme.textMuted
                    }

                    MouseArea {
                        id: closeMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.close()
                    }
                }
            }
        }

        // Body Content
        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: Theme.panelPadding
            spacing: Theme.spacingMedium

            Text {
                text: {
                    if (root.deleteMode === "playlist") {
                        return "Are you sure you want to delete playlist \"" + root.playlistToDelete + "\"? Tracks within your library database will remain intact.";
                    }
                    if (root.deleteMode === "database") {
                        return "This will purge all indexed tracks, albums, listening history, and cached artwork from the local library database. Audio files on disk will NOT be modified.";
                    }
                    return "This will permanently delete the selected audio file(s) from your storage device and library database. This action CANNOT be undone.";
                }
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.textMuted
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Rectangle {
                visible: root.deleteMode === "files"
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(130, Math.max(40, root.tracksToDelete.length * 24 + 12))
                color: Theme.surfaceElevated
                border.color: Theme.panelBorder
                border.width: 1
                radius: Theme.cornerRadiusSmall
                clip: true

                ListView {
                    id: fileList
                    anchors.fill: parent
                    anchors.margins: 6
                    model: root.tracksToDelete
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                    delegate: Text {
                        width: fileList.width - 12
                        text: (modelData.artist ? (modelData.artist + " - ") : "") + (modelData.title || modelData.filePath || "")
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.textSecondary
                        elide: Text.ElideMiddle
                    }
                }
            }

            // Footer Actions
            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: Theme.spacingSmall
                spacing: Theme.spacingMedium

                Item { Layout.fillWidth: true }

                Rectangle {
                    Layout.preferredWidth: 84
                    Layout.preferredHeight: 30
                    radius: Theme.cornerRadiusSmall
                    color: cancelMouse.containsMouse ? Theme.surfaceElevated : "transparent"
                    border.color: cancelMouse.containsMouse ? Theme.accent : Theme.panelBorder
                    clip: true

                    Text {
                        anchors.centerIn: parent
                        text: "Cancel"
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.textSecondary
                    }

                    MouseArea {
                        id: cancelMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.close()
                    }
                }

                Rectangle {
                    Layout.preferredWidth: root.deleteMode === "database" ? 140 : 150
                    Layout.preferredHeight: 30
                    radius: Theme.cornerRadiusSmall
                    color: deleteMouse.containsMouse ? Qt.darker(Theme.error, 1.2) : Theme.error
                    clip: true

                    RowLayout {
                        anchors.centerIn: parent
                        spacing: 6
                        VectorIcon {
                            name: "trash"
                            width: 11
                            height: 11
                            color: Theme.textPrimary
                        }
                        Text {
                            text: {
                                if (root.deleteMode === "playlist") return "Delete Playlist";
                                if (root.deleteMode === "database") return "Purge Database";
                                return "Delete Permanently";
                            }
                            font.pixelSize: Theme.fontSizeSmall
                            font.bold: true
                            color: Theme.textPrimary
                        }
                    }

                    MouseArea {
                        id: deleteMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (root.deleteMode === "playlist") {
                                var pl = root.playlistToDelete;
                                root.close();
                                bridge.deletePlaylist(pl);
                                root.playlistDeleted(pl);
                            } else if (root.deleteMode === "database") {
                                root.close();
                                bridge.clearLibrary();
                                root.databasePurged();
                            } else {
                                var ids = [];
                                for (var i = 0; i < root.tracksToDelete.length; ++i) {
                                    if (root.tracksToDelete[i].id) {
                                        ids.push(root.tracksToDelete[i].id);
                                    }
                                }
                                root.close();
                                if (ids.length > 0) {
                                    bridge.deleteFilesFromDisk(ids);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    ResizeGrip {
        target: root
        minWidth: 400
        minHeight: 200
    }
}
}
