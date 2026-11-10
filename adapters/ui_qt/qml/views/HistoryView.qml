import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../components"

Item {
    id: root

    property var historySections: bridge.getListeningHistoryGrouped()

    function refresh() {
        historySections = bridge.getListeningHistoryGrouped();
    }

    Connections {
        target: bridge
        function onQueueChanged() { root.refresh(); }
        function onPlaybackChanged() { root.refresh(); }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // History Header
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 52
            color: Theme.surface
            border.color: Theme.panelBorder
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                spacing: 12

                VectorIcon {
                    name: "queue"
                    width: 18
                    height: 18
                    color: Theme.accent
                }

                ColumnLayout {
                    spacing: 2
                    Text {
                        text: "Listening History & Timeline"
                        font.pixelSize: Theme.fontSizeLarge
                        font.bold: true
                        color: Theme.textPrimary
                    }
                    Text {
                        text: "Detailed log of recently played tracks grouped chronologically"
                        font.pixelSize: 10
                        color: Theme.textMuted
                    }
                }

                Item { Layout.fillWidth: true }

                // Refresh Button
                Rectangle {
                    Layout.preferredWidth: refreshHistoryText.implicitWidth + 24
                    Layout.preferredHeight: 28
                    Layout.alignment: Qt.AlignVCenter
                    radius: Theme.cornerRadiusSmall
                    color: refreshMouse.containsMouse ? Theme.selection : Theme.surfaceElevated
                    border.color: Theme.panelBorder

                    RowLayout {
                        anchors.centerIn: parent
                        spacing: 6
                        Text {
                            id: refreshHistoryText
                            text: "Refresh"
                            font.pixelSize: Theme.fontSizeSmall
                            color: Theme.textSecondary
                        }
                    }

                    MouseArea {
                        id: refreshMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.refresh()
                    }
                }

                // Clear History Button
                Rectangle {
                    Layout.preferredWidth: clearHistoryText.implicitWidth + 28
                    Layout.preferredHeight: 28
                    Layout.alignment: Qt.AlignVCenter
                    radius: Theme.cornerRadiusSmall
                    color: clearHistoryMouse.containsMouse ? Theme.selection : Theme.surfaceElevated
                    border.color: Theme.panelBorder

                    RowLayout {
                        anchors.centerIn: parent
                        spacing: 6
                        VectorIcon {
                            name: "clear"
                            width: 10
                            height: 10
                            color: Theme.accent
                        }
                        Text {
                            id: clearHistoryText
                            text: "Clear History"
                            font.pixelSize: Theme.fontSizeSmall
                            color: Theme.accent
                        }
                    }

                    MouseArea {
                        id: clearHistoryMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            bridge.clearPlaybackHistory();
                            root.refresh();
                        }
                    }
                }
            }
        }

        // Table Column Header Row
        Rectangle {
            Layout.fillWidth: true
            height: 28
            color: Theme.surfaceElevated
            border.color: Theme.panelBorder
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 8

                Item { Layout.preferredWidth: 32; Layout.fillHeight: true; clip: true; Text { anchors.verticalCenter: parent.verticalCenter; text: "#"; font.pixelSize: 9; font.bold: true; color: Theme.textMuted } }
                Item { Layout.preferredWidth: 24; Layout.fillHeight: true; clip: true; Text { anchors.verticalCenter: parent.verticalCenter; text: "♥"; font.pixelSize: 10; color: Theme.textMuted } }
                Item { Layout.fillWidth: true; Layout.preferredWidth: 200; Layout.minimumWidth: 100; Layout.fillHeight: true; clip: true; Text { anchors.verticalCenter: parent.verticalCenter; text: "TITLE"; font.pixelSize: 9; font.bold: true; color: Theme.textMuted } }
                Item { Layout.preferredWidth: 140; Layout.minimumWidth: 80; Layout.fillHeight: true; clip: true; Text { anchors.verticalCenter: parent.verticalCenter; text: "ARTIST"; font.pixelSize: 9; font.bold: true; color: Theme.textMuted } }
                Item { Layout.preferredWidth: 140; Layout.minimumWidth: 80; Layout.fillHeight: true; clip: true; Text { anchors.verticalCenter: parent.verticalCenter; text: "ALBUM"; font.pixelSize: 9; font.bold: true; color: Theme.textMuted } }
                Item { Layout.preferredWidth: 70; Layout.fillHeight: true; clip: true; Text { anchors.verticalCenter: parent.verticalCenter; text: "PLAYED"; font.pixelSize: 9; font.bold: true; color: Theme.textMuted } }
                Item { Layout.preferredWidth: 50; Layout.fillHeight: true; clip: true; Text { anchors.verticalCenter: parent.verticalCenter; text: "TIME"; font.pixelSize: 9; font.bold: true; color: Theme.textMuted } }
            }
        }

        // Timeline ScrollView
        ScrollView {
            id: histScrollView
            Layout.fillWidth: true
            Layout.fillHeight: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ScrollBar.vertical.policy: ScrollBar.AsNeeded
            clip: true

            ListView {
                id: sectionListView
                width: histScrollView.availableWidth
                model: root.historySections
                spacing: 12

                delegate: ColumnLayout {
                    width: sectionListView.width
                    spacing: 4

                    // Section Header (Today, Yesterday, This Week, Older)
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 28
                        color: Theme.surface
                        border.color: Theme.panelBorder
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            spacing: 8

                            Text {
                                text: modelData.section || "Recent"
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                color: Theme.accent
                            }

                            Rectangle {
                                width: countText.implicitWidth + 12
                                height: 16
                                radius: 8
                                color: Theme.surfaceElevated
                                border.color: Theme.panelBorder
                                Text {
                                    id: countText
                                    anchors.centerIn: parent
                                    text: "" + (modelData.items ? modelData.items.length : 0)
                                    font.pixelSize: 9
                                    font.bold: true
                                    color: Theme.textMuted
                                }
                            }

                            Item { Layout.fillWidth: true }
                        }
                    }

                    // Section Items
                    Repeater {
                        model: modelData.items || []

                        delegate: Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 28
                            color: histRowMouse.containsMouse ? Theme.surfaceElevated : (index % 2 === 0 ? Theme.surface : Theme.background)

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 12
                                anchors.rightMargin: 12
                                spacing: 8

                                Item {
                                    Layout.preferredWidth: 32
                                    Layout.fillHeight: true
                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: index + 1
                                        font.pixelSize: 10
                                        color: Theme.textMuted
                                    }
                                }

                                Item {
                                    Layout.preferredWidth: 24
                                    Layout.fillHeight: true
                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: modelData.isFavorite ? "♥" : "♡"
                                        font.pixelSize: 12
                                        color: modelData.isFavorite ? Theme.accent : Theme.textMuted
                                        opacity: modelData.isFavorite ? 1.0 : 0.3
                                    }
                                }

                                Item {
                                    Layout.fillWidth: true
                                    Layout.preferredWidth: 200
                                    Layout.minimumWidth: 100
                                    Layout.fillHeight: true
                                    clip: true
                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        text: modelData.title || "—"
                                        font.pixelSize: 11
                                        color: Theme.textPrimary
                                        elide: Text.ElideRight
                                    }
                                }

                                Item {
                                    Layout.preferredWidth: 140
                                    Layout.minimumWidth: 80
                                    Layout.fillHeight: true
                                    clip: true
                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        text: modelData.artist || "—"
                                        font.pixelSize: 11
                                        color: Theme.textSecondary
                                        elide: Text.ElideRight
                                    }
                                }

                                Item {
                                    Layout.preferredWidth: 140
                                    Layout.minimumWidth: 80
                                    Layout.fillHeight: true
                                    clip: true
                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        text: modelData.album || "—"
                                        font.pixelSize: 11
                                        color: Theme.textMuted
                                        elide: Text.ElideRight
                                    }
                                }

                                Item {
                                    Layout.preferredWidth: 70
                                    Layout.fillHeight: true
                                    clip: true
                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: modelData.playedAtFormatted || ""
                                        font.pixelSize: 10
                                        color: Theme.accentHover
                                    }
                                }

                                Item {
                                    Layout.preferredWidth: 50
                                    Layout.fillHeight: true
                                    clip: true
                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: modelData.durationFormatted || ""
                                        font.pixelSize: 10
                                        color: Theme.textMuted
                                    }
                                }
                            }

                            MouseArea {
                                id: histRowMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onDoubleClicked: {
                                    if (modelData.id) {
                                        bridge.playTrack(modelData.id);
                                    } else if (modelData.filePath) {
                                        bridge.openAudioFile(modelData.filePath);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
