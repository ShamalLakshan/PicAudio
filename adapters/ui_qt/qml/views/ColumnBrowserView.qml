import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

Rectangle {
    id: root

    property bool showBrowser: false
    property string selectedGenreFilter: "All"
    property string selectedArtistFilter: "All"

    signal genreFilterChanged(string genre)
    signal artistFilterChanged(string artist)
    signal requestAlbumContextMenu(string title, string artist)
    signal requestSwitchToExpandedView(string title, string artist)

    Layout.fillWidth: true
    Layout.preferredHeight: showBrowser ? bridge.columnBrowserHeight : 0
    visible: showBrowser
    color: Theme.background
    clip: true


    RowLayout {
        anchors.fill: parent
        anchors.bottomMargin: 1
        spacing: 1

        // Column 1: Genre Filter
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.surface

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                Rectangle {
                    Layout.fillWidth: true
                    height: 20
                    color: Theme.surfaceElevated
                    Text { anchors.centerIn: parent; text: "GENRE"; font.pixelSize: 9; font.bold: true; color: Theme.textMuted }
                }

                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: ["All (" + bridge.totalTracks + ")"].concat(bridge.genresList)

                    delegate: Rectangle {
                        width: parent ? parent.width : (ListView.view ? ListView.view.width : 0)
                        height: 20
                        clip: true
                        color: (root.selectedGenreFilter === modelData || (modelData.indexOf("All") === 0 && root.selectedGenreFilter === "All")) ? Theme.selection : (gMouse.containsMouse ? Theme.surfaceElevated : "transparent")

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 8
                            anchors.rightMargin: 8
                            text: modelData
                            font.pixelSize: 10
                            color: (root.selectedGenreFilter === modelData || (modelData.indexOf("All") === 0 && root.selectedGenreFilter === "All")) ? Theme.accent : Theme.textSecondary
                            elide: Text.ElideRight
                        }

                        MouseArea {
                            id: gMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (modelData.indexOf("All") === 0) {
                                    root.selectedGenreFilter = "All";
                                    bridge.filterByGenre("All");
                                    root.genreFilterChanged("All");
                                } else {
                                    root.selectedGenreFilter = modelData;
                                    bridge.filterByGenre(modelData);
                                    root.genreFilterChanged(modelData);
                                }
                            }
                        }
                    }
                }
            }
        }

        // Column 2: Artist Filter
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.surface

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                Rectangle {
                    Layout.fillWidth: true
                    height: 20
                    color: Theme.surfaceElevated
                    Text { anchors.centerIn: parent; text: "ARTIST"; font.pixelSize: 9; font.bold: true; color: Theme.textMuted }
                }

                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: ["All (" + bridge.artistsList.length + ")"].concat(bridge.artistsList)

                    delegate: Rectangle {
                        width: parent ? parent.width : (ListView.view ? ListView.view.width : 0)
                        height: 20
                        clip: true
                        color: (root.selectedArtistFilter === modelData || (modelData.indexOf("All") === 0 && root.selectedArtistFilter === "All")) ? Theme.selection : (aMouse.containsMouse ? Theme.surfaceElevated : "transparent")

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 8
                            anchors.rightMargin: 8
                            text: modelData
                            font.pixelSize: 10
                            color: (root.selectedArtistFilter === modelData || (modelData.indexOf("All") === 0 && root.selectedArtistFilter === "All")) ? Theme.accent : Theme.textSecondary
                            elide: Text.ElideRight
                        }

                        MouseArea {
                            id: aMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (modelData.indexOf("All") === 0) {
                                    root.selectedArtistFilter = "All";
                                    bridge.filterByArtist("All");
                                    root.artistFilterChanged("All");
                                } else {
                                    root.selectedArtistFilter = modelData;
                                    bridge.filterByArtist(modelData);
                                    root.artistFilterChanged(modelData);
                                }
                            }
                        }
                    }
                }
            }
        }

        // Column 3: Album Filter
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.surface

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                Rectangle {
                    Layout.fillWidth: true
                    height: 20
                    color: Theme.surfaceElevated
                    Text { anchors.centerIn: parent; text: "ALBUM"; font.pixelSize: 9; font.bold: true; color: Theme.textMuted }
                }

                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: bridge.albumModel

                    delegate: Rectangle {
                        width: parent ? parent.width : (ListView.view ? ListView.view.width : 0)
                        height: 20
                        color: albColMouse.containsMouse ? Theme.surfaceElevated : "transparent"

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 8
                            anchors.rightMargin: 8
                            spacing: 4

                            Text {
                                text: model.title
                                font.pixelSize: 10
                                color: Theme.textSecondary
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }

                            Text {
                                text: model.trackCount
                                font.pixelSize: 9
                                color: Theme.textMuted
                            }

                            VectorIcon {
                                visible: albColMouse.containsMouse
                                name: "album_tracks"
                                width: 10
                                height: 10
                                color: Theme.accent
                            }
                        }

                        MouseArea {
                            id: albColMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            acceptedButtons: Qt.LeftButton | Qt.RightButton
                            onClicked: (mouse) => {
                                if (mouse.button === Qt.RightButton) {
                                    root.requestAlbumContextMenu(model.title, model.artist);
                                } else {
                                    bridge.search(model.title);
                                }
                            }
                            onDoubleClicked: {
                                root.requestSwitchToExpandedView(model.title, model.artist);
                            }
                            ToolTip.visible: albColMouse.containsMouse
                            ToolTip.text: "Click to filter • Right-click for album menu • Double-click to expand"
                        }
                    }
                }
            }
        }
    }

    // 1px bottom divider
    Rectangle {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 1
        color: Theme.panelBorder
    }
}
