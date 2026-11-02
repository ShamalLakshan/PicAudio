import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../views"

Rectangle {
    id: root

    property int currentTab: 0
    signal tabChanged(int tab)

    Layout.preferredWidth: bridge.rightPanelWidth
    Layout.fillHeight: true
    color: Theme.surface

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Tabs Header
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 28
            color: Theme.surfaceElevated
            border.color: Theme.panelBorder
            border.width: 1

            RowLayout {
                anchors.fill: parent
                spacing: 0

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: root.currentTab === 0 ? Theme.selection : "transparent"
                    Text {
                        anchors.centerIn: parent
                        text: "PROPERTIES"
                        font.pixelSize: Theme.fontSizeSmall - 1
                        font.bold: true
                        font.letterSpacing: 1.0
                        color: root.currentTab === 0 ? Theme.textPrimary : Theme.textMuted
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            root.currentTab = 0;
                            root.tabChanged(0);
                        }
                    }
                }

                Rectangle { width: 1; height: 16; color: Theme.panelBorder }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: root.currentTab === 1 ? Theme.selection : "transparent"
                    Text {
                        anchors.centerIn: parent
                        text: "PLAY QUEUE"
                        font.pixelSize: Theme.fontSizeSmall - 1
                        font.bold: true
                        font.letterSpacing: 1.0
                        color: root.currentTab === 1 ? Theme.textPrimary : Theme.textMuted
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            root.currentTab = 1;
                            root.tabChanged(1);
                        }
                    }
                }
            }
        }

        // Track properties tab
        ScrollView {
            id: inspectorScrollView
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: root.currentTab === 0
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ScrollBar.vertical.policy: ScrollBar.AsNeeded
            contentWidth: availableWidth

            ColumnLayout {
                width: inspectorScrollView.availableWidth
                spacing: 12

                // Album art
                Item { height: 4; width: 1 }
                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: Math.min(260, Math.max(120, inspectorScrollView.availableWidth - 32))
                    Layout.preferredHeight: Layout.preferredWidth
                    color: Theme.background
                    border.color: Theme.panelBorder
                    border.width: 1
                    clip: true

                    Image {
                        anchors.fill: parent
                        source: bridge.currentArtUrl
                        fillMode: Image.PreserveAspectCrop
                    }
                }

                // Track Title & Artist
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    spacing: 2

                    Text {
                        text: bridge.currentTrackTitle
                        font.pixelSize: Theme.fontSizeLarge
                        font.bold: true
                        color: Theme.textPrimary
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    Text {
                        text: bridge.currentArtist
                        font.pixelSize: Theme.fontSizeBase
                        font.bold: true
                        color: Theme.accent
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    Text {
                        text: bridge.currentAlbum + (bridge.currentYear.length > 0 ? " (" + bridge.currentYear + ")" : "")
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.textMuted
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }

                // Dense Technical Specs Table
                Rectangle {
                    Layout.fillWidth: true
                    Layout.leftMargin: 12
                    Layout.rightMargin: 12
                    Layout.preferredHeight: specGrid.implicitHeight + 16
                    color: Theme.surfaceElevated
                    border.color: Theme.panelBorder
                    border.width: 1
                    radius: 3

                    GridLayout {
                        id: specGrid
                        anchors.fill: parent
                        anchors.margins: 10
                        columns: 2
                        rowSpacing: 5
                        columnSpacing: 10

                        Text { text: "Container:"; font.pixelSize: Theme.fontSizeSmall - 1; font.bold: true; color: Theme.textMuted }
                        Text { text: bridge.currentCodec.length > 0 ? bridge.currentCodec : "—"; font.pixelSize: Theme.fontSizeSmall - 1; color: Theme.textPrimary; elide: Text.ElideRight; Layout.fillWidth: true }

                        Text { text: "Bit Depth:"; font.pixelSize: Theme.fontSizeSmall - 1; font.bold: true; color: Theme.textMuted }
                        Text { text: bridge.currentBitDepth > 0 ? bridge.currentBitDepth + "-bit" : "—"; font.pixelSize: Theme.fontSizeSmall - 1; color: Theme.accent; elide: Text.ElideRight; Layout.fillWidth: true }

                        Text { text: "Sample Rate:"; font.pixelSize: Theme.fontSizeSmall - 1; font.bold: true; color: Theme.textMuted }
                        Text { text: bridge.currentSampleRate > 0 ? bridge.currentSampleRate + " Hz" : "—"; font.pixelSize: Theme.fontSizeSmall - 1; color: Theme.accent; elide: Text.ElideRight; Layout.fillWidth: true }

                        Text { text: "Bitrate:"; font.pixelSize: Theme.fontSizeSmall - 1; font.bold: true; color: Theme.textMuted }
                        Text { text: bridge.currentBitrate > 0 ? bridge.currentBitrate + " kbps" : "—"; font.pixelSize: Theme.fontSizeSmall - 1; color: Theme.textPrimary; elide: Text.ElideRight; Layout.fillWidth: true }

                        Text { text: "Channels:"; font.pixelSize: Theme.fontSizeSmall - 1; font.bold: true; color: Theme.textMuted }
                        Text { text: bridge.currentChannels === 2 ? "Stereo (2.0)" : (bridge.currentChannels === 1 ? "Mono" : (bridge.currentChannels > 2 ? bridge.currentChannels + " Ch" : "—")); font.pixelSize: Theme.fontSizeSmall - 1; color: Theme.textPrimary; elide: Text.ElideRight; Layout.fillWidth: true }

                        Text { text: "File Size:"; font.pixelSize: Theme.fontSizeSmall - 1; font.bold: true; color: Theme.textMuted }
                        Text { text: bridge.currentFileSizeStr.length > 0 ? bridge.currentFileSizeStr : "—"; font.pixelSize: Theme.fontSizeSmall - 1; color: Theme.textPrimary; elide: Text.ElideRight; Layout.fillWidth: true }
                    }
                }

                // Full File Path Box
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 14
                    Layout.rightMargin: 14
                    spacing: 3

                    Text {
                        text: "FILE LOCATION"
                        font.pixelSize: Theme.fontSizeSmall - 2
                        font.bold: true
                        font.letterSpacing: 1.0
                        color: Theme.textMuted
                    }

                    Text {
                        text: bridge.currentFilePath.length > 0 ? bridge.currentFilePath : "No file loaded"
                        font.pixelSize: Theme.fontSizeSmall - 1
                        color: Theme.textMuted
                        wrapMode: Text.WrapAnywhere
                        Layout.fillWidth: true
                    }
                }

                Item { height: 10; width: 1 }
            }
        }

        // Play queue tab
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: root.currentTab === 1

            QueueView {
                anchors.fill: parent
            }
        }
    }
}
