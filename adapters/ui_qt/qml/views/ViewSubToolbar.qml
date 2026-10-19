import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

Rectangle {
    id: root

    property int mainViewMode: 1
    property bool showColumnBrowser: false
    property string activeLetterFilter: "All"

    signal requestSwitchViewMode(int mode)
    signal toggleColumnBrowser()
    signal letterFilterSelected(string letter)

    Layout.fillWidth: true
    Layout.preferredHeight: 34
    color: Theme.surfaceElevated
    border.color: Theme.panelBorder
    border.width: 1

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        spacing: 10

        // View Mode Toggle Icons
        RowLayout {
            spacing: 2
            Layout.alignment: Qt.AlignVCenter

            Rectangle {
                Layout.preferredWidth: 26
                Layout.preferredHeight: 24
                width: 26
                height: 24
                radius: 2
                color: root.mainViewMode === 1 ? Theme.selection : (vm1Mouse.containsMouse ? Theme.surfaceElevated : "transparent")
                border.color: root.mainViewMode === 1 ? Theme.accent : "transparent"
                VectorIcon { anchors.centerIn: parent; name: "table"; width: 12; height: 12; color: root.mainViewMode === 1 ? Theme.accent : Theme.textMuted }
                MouseArea {
                    id: vm1Mouse; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: root.requestSwitchViewMode(1)
                }
                ToolTip.visible: vm1Mouse.containsMouse
                ToolTip.text: "Track Details Table (Alt+1)"
            }

            Rectangle {
                Layout.preferredWidth: 26
                Layout.preferredHeight: 24
                width: 26
                height: 24
                radius: 2
                color: root.mainViewMode === 0 ? Theme.selection : (vm0Mouse.containsMouse ? Theme.surfaceElevated : "transparent")
                border.color: root.mainViewMode === 0 ? Theme.accent : "transparent"
                VectorIcon { anchors.centerIn: parent; name: "grid"; width: 12; height: 12; color: root.mainViewMode === 0 ? Theme.accent : Theme.textMuted }
                MouseArea {
                    id: vm0Mouse; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: root.requestSwitchViewMode(0)
                }
                ToolTip.visible: vm0Mouse.containsMouse
                ToolTip.text: "Album Grid View (Alt+2)"
            }

            Rectangle {
                Layout.preferredWidth: 26
                Layout.preferredHeight: 24
                width: 26
                height: 24
                radius: 2
                color: root.mainViewMode === 2 ? Theme.selection : (vm2Mouse.containsMouse ? Theme.surfaceElevated : "transparent")
                border.color: root.mainViewMode === 2 ? Theme.accent : "transparent"
                VectorIcon { anchors.centerIn: parent; name: "album_tracks"; width: 12; height: 12; color: root.mainViewMode === 2 ? Theme.accent : Theme.textMuted }
                MouseArea {
                    id: vm2Mouse; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: root.requestSwitchViewMode(2)
                }
                ToolTip.visible: vm2Mouse.containsMouse
                ToolTip.text: "Album Details & Tracks (Alt+3)"
            }

            Rectangle {
                Layout.preferredWidth: 26
                Layout.preferredHeight: 24
                width: 26
                height: 24
                radius: 2
                color: root.mainViewMode === 4 ? Theme.selection : (vm4Mouse.containsMouse ? Theme.surfaceElevated : "transparent")
                border.color: root.mainViewMode === 4 ? Theme.accent : "transparent"
                VectorIcon { anchors.centerIn: parent; name: "disc"; width: 12; height: 12; color: root.mainViewMode === 4 ? Theme.accent : Theme.textMuted }
                MouseArea {
                    id: vm4Mouse; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: root.requestSwitchViewMode(4)
                }
                ToolTip.visible: vm4Mouse.containsMouse
                ToolTip.text: "Now Playing View (Alt+4)"
            }
        }

        Rectangle { Layout.preferredWidth: 1; Layout.preferredHeight: 16; width: 1; height: 16; color: Theme.panelBorder }

        // Toggle Category Filters Button
        Rectangle {
            Layout.preferredWidth: colBtnLayout.implicitWidth + 14
            Layout.preferredHeight: 22
            Layout.alignment: Qt.AlignVCenter
            width: Layout.preferredWidth
            height: 22
            radius: 2
            color: root.showColumnBrowser ? Theme.selection : (cbMouse.containsMouse ? Theme.surfaceElevated : "transparent")
            border.color: root.showColumnBrowser ? Theme.accent : Theme.panelBorder

            RowLayout {
                id: colBtnLayout
                anchors.centerIn: parent
                spacing: 4

                VectorIcon {
                    name: "filter"
                    width: 10
                    height: 10
                    color: root.showColumnBrowser ? Theme.accent : Theme.textMuted
                }

                Text {
                    text: "Filters"
                    font.pixelSize: 10
                    font.bold: root.showColumnBrowser
                    color: root.showColumnBrowser ? Theme.accentHover : Theme.textSecondary
                }

                Text {
                    text: root.showColumnBrowser ? "▲" : "▼"
                    font.pixelSize: 8
                    color: root.showColumnBrowser ? Theme.accent : Theme.textMuted
                }
            }

            MouseArea {
                id: cbMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.toggleColumnBrowser()
            }

            ToolTip.visible: cbMouse.containsMouse
            ToolTip.text: (root.showColumnBrowser ? "Collapse" : "Expand") + " Category Filters (Genre, Artist, Album) (Ctrl+2)"
        }

        // Alphabet jump ribbon
        Flickable {
            Layout.fillWidth: true
            Layout.preferredHeight: 24
            contentWidth: alphaRow.implicitWidth
            clip: true

            RowLayout {
                id: alphaRow
                spacing: 1

                Repeater {
                    model: ["All", "#", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z"]

                    delegate: Rectangle {
                        Layout.preferredWidth: alphaText.implicitWidth + 8
                        Layout.preferredHeight: 20
                        width: Layout.preferredWidth
                        height: 20
                        radius: 2
                        color: root.activeLetterFilter === modelData ? Theme.accent : (alphaMouse.containsMouse ? Theme.surfaceElevated : "transparent")

                        Text {
                            id: alphaText
                            anchors.centerIn: parent
                            text: modelData
                            font.pixelSize: 10
                            font.bold: root.activeLetterFilter === modelData
                            color: root.activeLetterFilter === modelData ? Theme.textPrimary : Theme.textMuted
                        }

                        MouseArea {
                            id: alphaMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                root.activeLetterFilter = modelData;
                                root.letterFilterSelected(modelData);
                            }
                        }
                    }
                }
            }
        }

        // Item counter
        Text {
            text: {
                if (root.mainViewMode === 0) {
                    return bridge.albumModel.count + " albums";
                } else if (root.mainViewMode === 2) {
                    return (bridge.selectedAlbumTrackCount > 0 ? (bridge.selectedAlbumTrackCount + " tracks • " + bridge.selectedAlbumDuration) : "Album Details");
                } else if (root.mainViewMode === 3) {
                    return bridge.historyTrackModel.count + " played";
                }
                return bridge.trackModel.count + " tracks";
            }
            font.pixelSize: 10
            color: Theme.textMuted
        }
    }
}
