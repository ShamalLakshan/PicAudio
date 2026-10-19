import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

Item {
    id: root

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // History Header
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 46
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
                    width: 16
                    height: 16
                    color: Theme.accent
                }

                ColumnLayout {
                    spacing: 2
                    Text {
                        text: "Listening History"
                        font.pixelSize: Theme.fontSizeLarge
                        font.bold: true
                        color: Theme.textPrimary
                    }
                    Text {
                        text: bridge.historyTrackModel.count + " tracks played in recent sessions"
                        font.pixelSize: 10
                        color: Theme.textMuted
                    }
                }

                Item { Layout.fillWidth: true }

                // Clear History Button
                Rectangle {
                    width: clearHistoryText.implicitWidth + 24
                    height: 28
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
                            color: Theme.textSecondary
                        }
                        Text {
                            id: clearHistoryText
                            text: "Clear History"
                            font.pixelSize: Theme.fontSizeSmall
                            color: Theme.textSecondary
                        }
                    }

                    MouseArea {
                        id: clearHistoryMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            bridge.clearPlaybackHistory();
                        }
                    }
                }
            }
        }

        // Table Header Row
        Rectangle {
            Layout.fillWidth: true
            height: 26
            color: Theme.surfaceElevated
            border.color: Theme.panelBorder
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: 8

                Item { Layout.preferredWidth: 28; Layout.fillHeight: true; clip: true; Text { anchors.verticalCenter: parent.verticalCenter; text: "#"; font.pixelSize: 9; font.bold: true; color: Theme.textMuted } }
                Item { Layout.fillWidth: true; Layout.preferredWidth: 200; Layout.minimumWidth: 100; Layout.fillHeight: true; clip: true; Text { anchors.verticalCenter: parent.verticalCenter; text: "TITLE"; font.pixelSize: 9; font.bold: true; color: Theme.textMuted } }
                Item { Layout.preferredWidth: 140; Layout.minimumWidth: 80; Layout.fillHeight: true; clip: true; Text { anchors.verticalCenter: parent.verticalCenter; text: "ARTIST"; font.pixelSize: 9; font.bold: true; color: Theme.textMuted } }
                Item { Layout.preferredWidth: 140; Layout.minimumWidth: 80; Layout.fillHeight: true; clip: true; Text { anchors.verticalCenter: parent.verticalCenter; text: "ALBUM"; font.pixelSize: 9; font.bold: true; color: Theme.textMuted } }
                Item { Layout.preferredWidth: 70; Layout.fillHeight: true; clip: true; Text { anchors.verticalCenter: parent.verticalCenter; text: "FORMAT"; font.pixelSize: 9; font.bold: true; color: Theme.textMuted } }
                Item { Layout.preferredWidth: 45; Layout.fillHeight: true; clip: true; Text { anchors.verticalCenter: parent.verticalCenter; text: "TIME"; font.pixelSize: 9; font.bold: true; color: Theme.textMuted } }
            }
        }

        // History List
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            ScrollBar.vertical.policy: ScrollBar.AsNeeded

            ListView {
                id: historyListView
                anchors.fill: parent
                model: bridge.historyTrackModel
                clip: true

                delegate: Rectangle {
                    width: historyListView.width
                    height: 26
                    color: histRowMouse.containsMouse ? Theme.surfaceElevated : (index % 2 === 0 ? Theme.surface : Theme.background)

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        spacing: 8

                        Item {
                            Layout.preferredWidth: 28
                            Layout.fillHeight: true
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: index + 1
                                font.pixelSize: 10
                                color: Theme.textMuted
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
                                text: model.title
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
                                text: model.artist
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
                                text: model.album
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
                                text: model.codec
                                font.pixelSize: 10
                                color: Theme.accent
                            }
                        }

                        Item {
                            Layout.preferredWidth: 45
                            Layout.fillHeight: true
                            clip: true
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: model.durationFormatted
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
                            bridge.openAudioFile(model.filePath);
                        }
                    }
                }
            }
        }
    }
}
