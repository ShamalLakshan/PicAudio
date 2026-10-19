import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

ScrollView {
    id: root

    signal albumSelected(string title, string artist)
    signal requestAlbumContextMenu(string title, string artist)

    ScrollBar.vertical.policy: ScrollBar.AsNeeded

    GridView {
        id: albumGridView
        anchors.fill: parent
        anchors.margins: 14
        property int minColWidth: 160
        property int numCols: Math.max(1, Math.floor((width - 4) / minColWidth))
        cellWidth: Math.floor((width - 4) / numCols)
        cellHeight: cellWidth + 68
        model: bridge.albumModel
        clip: true

        delegate: Rectangle {
            width: albumGridView.cellWidth - 10
            height: albumGridView.cellHeight - 10
            radius: 4
            color: albCardMouse.containsMouse ? Theme.surfaceElevated : Theme.surface
            border.color: albCardMouse.containsMouse ? Theme.accent : Theme.panelBorder
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 6

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: albumGridView.cellWidth - 26
                    color: Theme.background
                    border.color: Theme.panelBorder
                    clip: true

                    Image {
                        anchors.fill: parent
                        source: "image://albumart/" + (model.artHash ? model.artHash : model.id)
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                    }

                    // Hover Play Button
                    Rectangle {
                        anchors.bottom: parent.bottom
                        anchors.right: parent.right
                        anchors.margins: 6
                        width: 28
                        height: 28
                        radius: 14
                        color: Theme.accent
                        visible: albCardMouse.containsMouse

                        VectorIcon {
                            anchors.centerIn: parent
                            name: "play"
                            width: 12
                            height: 12
                            color: Theme.textPrimary
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: bridge.playAlbum(model.title, model.artist)
                        }
                    }
                }

                Text {
                    text: model.title
                    font.pixelSize: 11
                    font.bold: true
                    color: Theme.textPrimary
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                Text {
                    text: model.artist + (model.year > 0 ? " • " + model.year : "")
                    font.pixelSize: 10
                    color: Theme.textMuted
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                Text {
                    text: model.trackCount + " tracks • " + model.primaryCodec
                    font.pixelSize: 9
                    color: Theme.accent
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }

            MouseArea {
                id: albCardMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                onClicked: (mouse) => {
                    if (mouse.button === Qt.RightButton) {
                        root.requestAlbumContextMenu(model.title, model.artist);
                    } else {
                        root.albumSelected(model.title, model.artist);
                    }
                }
                onDoubleClicked: {
                    bridge.playAlbumNow(model.title, model.artist);
                }
            }
        }
    }
}
