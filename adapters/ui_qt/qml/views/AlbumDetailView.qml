import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

Item {
    id: root

    property int previousViewMode: 0

    signal requestSwitchViewMode(int mode)
    signal requestSwitchToExpandedView(string title, string artist)
    signal requestAlbumContextMenu(string title, string artist)
    signal requestTrackContextMenu(var track)

    // Empty State if no album is selected
    Rectangle {
        anchors.fill: parent
        color: Theme.surface
        visible: bridge.selectedAlbumTitle.length === 0

        ColumnLayout {
            anchors.centerIn: parent
            spacing: 14

            VectorIcon {
                Layout.alignment: Qt.AlignHCenter
                name: "album_tracks"
                width: 44
                height: 44
                color: Theme.textMuted
            }

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: "No Album Selected"
                font.pixelSize: 15
                font.bold: true
                color: Theme.textPrimary
            }

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: "Choose an album from the Album Grid or Track List to view its artwork and complete tracklist."
                font.pixelSize: 11
                color: Theme.textMuted
            }

            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 10

                Rectangle {
                    width: emptyBrowseText.implicitWidth + 24
                    height: 30
                    radius: 3
                    color: emptyBrowseMouse.containsMouse ? Theme.accentHover : Theme.accent

                    RowLayout {
                        anchors.centerIn: parent
                        spacing: 6
                        VectorIcon { name: "grid"; width: 12; height: 12; color: Theme.textPrimary }
                        Text { id: emptyBrowseText; text: "Browse Album Grid"; font.pixelSize: 11; font.bold: true; color: Theme.textPrimary }
                    }

                    MouseArea {
                        id: emptyBrowseMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.requestSwitchViewMode(0)
                    }
                }

                Rectangle {
                    visible: bridge.currentAlbum.length > 0
                    width: emptyNowPlayingText.implicitWidth + 24
                    height: 30
                    radius: 3
                    color: emptyNpMouse.containsMouse ? Theme.selection : Theme.surfaceElevated
                    border.color: Theme.panelBorder

                    RowLayout {
                        anchors.centerIn: parent
                        spacing: 6
                        VectorIcon { name: "play"; width: 10; height: 10; color: Theme.accent }
                        Text { id: emptyNowPlayingText; text: "View Playing Album"; font.pixelSize: 11; color: Theme.textPrimary }
                    }

                    MouseArea {
                        id: emptyNpMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (bridge.currentAlbum.length > 0) {
                                root.requestSwitchToExpandedView(bridge.currentAlbum, bridge.currentArtist);
                            }
                        }
                    }
                }
            }
        }
    }

    // Loaded Album Details
    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        visible: bridge.selectedAlbumTitle.length > 0

        // Album header
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 148
            color: Theme.surface
            border.color: Theme.panelBorder
            border.width: 1

            ColumnLayout {
                id: detailHeaderCol
                anchors.fill: parent
                anchors.margins: 12
                spacing: 8

                // Breadcrumb navigation row
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Rectangle {
                        width: backBreadcrumbText.implicitWidth + 16
                        height: 22
                        radius: 2
                        color: backBreadcrumbMouse.containsMouse ? Theme.selection : Theme.surfaceElevated
                        border.color: Theme.panelBorder

                        RowLayout {
                            anchors.centerIn: parent
                            spacing: 4
                            VectorIcon { name: "previous"; width: 9; height: 9; color: Theme.accent }
                            Text {
                                id: backBreadcrumbText
                                text: root.previousViewMode === 1 ? "Track List" : "Albums"
                                font.pixelSize: 10
                                font.bold: true
                                color: Theme.textSecondary
                            }
                        }

                        MouseArea {
                            id: backBreadcrumbMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.requestSwitchViewMode(root.previousViewMode)
                        }
                    }

                    Text {
                        text: "›"
                        font.pixelSize: 11
                        color: Theme.textMuted
                    }

                    Text {
                        text: bridge.selectedAlbumTitle
                        font.pixelSize: 10
                        font.bold: true
                        color: Theme.accent
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }

                // Main album info row
                RowLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 14

                    Rectangle {
                        width: 96
                        height: 96
                        color: Theme.background
                        border.color: Theme.panelBorder
                        clip: true

                        Image {
                            anchors.fill: parent
                            source: "image://albumart/" + (bridge.selectedAlbumArtHash ? bridge.selectedAlbumArtHash : ("alb_" + bridge.selectedAlbumTitle + "_" + bridge.selectedAlbumArtist))
                            fillMode: Image.PreserveAspectCrop
                        }

                        MouseArea {
                            anchors.fill: parent
                            acceptedButtons: Qt.RightButton
                            cursorShape: Qt.PointingHandCursor
                            onClicked: (mouse) => {
                                if (mouse.button === Qt.RightButton) {
                                    root.requestAlbumContextMenu(bridge.selectedAlbumTitle, bridge.selectedAlbumArtist);
                                }
                            }
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 3

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Text {
                                text: bridge.selectedAlbumTitle
                                font.pixelSize: 16
                                font.bold: true
                                color: Theme.textPrimary
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                        }

                        Text {
                            text: bridge.selectedAlbumArtist + (bridge.selectedAlbumYear ? " (" + bridge.selectedAlbumYear + ")" : "")
                            font.pixelSize: 12
                            color: Theme.accent
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        Text {
                            text: (bridge.selectedAlbumGenre ? bridge.selectedAlbumGenre + " • " : "") +
                                  bridge.selectedAlbumTrackCount + " Tracks • " +
                                  bridge.selectedAlbumDuration
                            font.pixelSize: 10
                            color: Theme.textMuted
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        Item { height: 2; width: 1 }

                        // Action Buttons Row
                        RowLayout {
                            spacing: 8

                            Rectangle {
                                width: albPlayBtnText.implicitWidth + 20
                                height: 24
                                radius: 2
                                color: albPlayMouse.containsMouse ? Theme.accentHover : Theme.accent

                                RowLayout {
                                    anchors.centerIn: parent
                                    spacing: 6
                                    VectorIcon { name: "play"; width: 10; height: 10; color: Theme.textPrimary }
                                    Text { id: albPlayBtnText; text: "Play Album"; font.pixelSize: 10; font.bold: true; color: Theme.textPrimary }
                                }

                                MouseArea {
                                    id: albPlayMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: bridge.playAlbum(bridge.selectedAlbumTitle, bridge.selectedAlbumArtist)
                                }
                            }

                            Rectangle {
                                width: albQueueBtnText.implicitWidth + 18
                                height: 24
                                radius: 2
                                color: albQueueMouse.containsMouse ? Theme.selection : Theme.surfaceElevated
                                border.color: Theme.panelBorder

                                RowLayout {
                                    anchors.centerIn: parent
                                    spacing: 5
                                    VectorIcon { name: "queue"; width: 10; height: 10; color: Theme.textSecondary }
                                    Text { id: albQueueBtnText; text: "Queue Album"; font.pixelSize: 10; color: Theme.textSecondary }
                                }

                                MouseArea {
                                    id: albQueueMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: bridge.queueAlbumLast(bridge.selectedAlbumTitle, bridge.selectedAlbumArtist)
                                }
                            }

                            Rectangle {
                                width: viewInTableText.implicitWidth + 18
                                height: 24
                                radius: 2
                                color: viewInTableMouse.containsMouse ? Theme.selection : Theme.surfaceElevated
                                border.color: Theme.panelBorder

                                RowLayout {
                                    anchors.centerIn: parent
                                    spacing: 5
                                    VectorIcon { name: "table"; width: 10; height: 10; color: Theme.textSecondary }
                                    Text { id: viewInTableText; text: "View in Track List"; font.pixelSize: 10; color: Theme.textSecondary }
                                }

                                MouseArea {
                                    id: viewInTableMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        bridge.search(bridge.selectedAlbumTitle);
                                        root.requestSwitchViewMode(1);
                                    }
                                }
                                ToolTip.visible: viewInTableMouse.containsMouse
                                ToolTip.text: "Filter and view these tracks in the main list table"
                            }

                            Rectangle {
                                width: backToGridText.implicitWidth + 16
                                height: 24
                                radius: 2
                                color: backGridMouse.containsMouse ? Theme.selection : Theme.surfaceElevated
                                border.color: Theme.panelBorder

                                RowLayout {
                                    anchors.centerIn: parent
                                    spacing: 5
                                    VectorIcon { name: "grid"; width: 10; height: 10; color: Theme.textSecondary }
                                    Text { id: backToGridText; text: "All Albums"; font.pixelSize: 10; color: Theme.textSecondary }
                                }

                                MouseArea {
                                    id: backGridMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: root.requestSwitchViewMode(0)
                                }
                            }
                        }
                    }
                }
            }
        }

        // Album Tracklist Table
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            ScrollBar.vertical.policy: ScrollBar.AsNeeded

            ListView {
                id: albumDetailListView
                anchors.fill: parent
                model: bridge.albumDetailTrackModel
                clip: true

                delegate: Rectangle {
                    width: albumDetailListView.width
                    height: 26
                    color: (bridge.currentFilePath === model.filePath) ? Theme.selection : (dtRowMouse.containsMouse ? Theme.surfaceElevated : (index % 2 === 0 ? Theme.surface : Theme.background))

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        spacing: 10

                        Text {
                            text: model.trackNumber > 0 ? model.trackNumber : (index + 1)
                            font.pixelSize: 10
                            color: (bridge.currentFilePath === model.filePath) ? Theme.accent : Theme.textMuted
                            Layout.preferredWidth: 28
                        }

                        VectorIcon {
                            visible: bridge.currentFilePath === model.filePath
                            name: bridge.isPlaying ? "volume" : "play"
                            width: 10
                            height: 10
                            color: Theme.accent
                            Layout.preferredWidth: 14
                        }

                        Text {
                            text: model.title
                            font.pixelSize: 11
                            font.bold: (bridge.currentFilePath === model.filePath)
                            color: (bridge.currentFilePath === model.filePath) ? Theme.textPrimary : Theme.textSecondary
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        Text {
                            text: model.artist
                            font.pixelSize: 10
                            color: Theme.textMuted
                            Layout.preferredWidth: 140
                            elide: Text.ElideRight
                        }

                        Text {
                            text: model.codec + " " + model.sampleRate / 1000 + "kHz"
                            font.pixelSize: 9
                            color: Theme.accent
                            Layout.preferredWidth: 90
                        }

                        Text {
                            text: model.durationFormatted
                            font.pixelSize: 10
                            color: Theme.textMuted
                            Layout.preferredWidth: 45
                        }
                    }

                    MouseArea {
                        id: dtRowMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        onDoubleClicked: bridge.playTrackFromDetail(index)
                        onClicked: (mouse) => {
                            if (mouse.button === Qt.RightButton) {
                                var trk = { id: model.id, title: model.title, artist: model.artist, filePath: model.filePath, album: model.album, genre: model.genre, year: model.year, trackNumber: model.trackNumber };
                                root.requestTrackContextMenu(trk);
                            }
                        }
                    }
                }
            }
        }
    }
}
