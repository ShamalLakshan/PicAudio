import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    id: jukeboxRoot

    RowLayout {
        anchors.fill: parent
        anchors.margins: Math.round(24 * Theme.uiScale)
        spacing: Math.round(24 * Theme.uiScale)

        // Left Side: Oversized Artwork Card + Now Playing Typography
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.surface
            border.color: Theme.panelBorder
            radius: Math.round(8 * Theme.uiScale)

            ColumnLayout {
                anchors.centerIn: parent
                spacing: Math.round(18 * Theme.uiScale)

                // Oversized Album Art
                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    width: Math.min(Math.round(340 * Theme.uiScale), jukeboxRoot.width * 0.4)
                    height: width
                    radius: Math.round(8 * Theme.uiScale)
                    color: Theme.background
                    border.color: Theme.panelBorder
                    clip: true

                    Image {
                        anchors.fill: parent
                        source: "image://albumart/" + bridge.currentTrackTitle
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                    }
                }

                // Title & Artist
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: bridge.currentTrackTitle.length > 0 ? bridge.currentTrackTitle : qsTr("PicAudio Jukebox")
                    color: Theme.textPrimary
                    font.pixelSize: Math.round(24 * Theme.uiScale)
                    font.bold: true
                    elide: Text.ElideRight
                }

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: bridge.currentArtist.length > 0 ? bridge.currentArtist : qsTr("Ready for playback")
                    color: Theme.accent
                    font.pixelSize: Math.round(18 * Theme.uiScale)
                    elide: Text.ElideRight
                }

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: bridge.currentAlbum.length > 0 ? bridge.currentAlbum : ""
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeBase
                    elide: Text.ElideRight
                }

                // Transport Deck (Touch / Presentation sized)
                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: Math.round(18 * Theme.uiScale)

                    Button {
                        text: "⏮"
                        font.pixelSize: Math.round(20 * Theme.uiScale)
                        Layout.preferredWidth: Math.round(52 * Theme.uiScale)
                        Layout.preferredHeight: Math.round(52 * Theme.uiScale)
                        onClicked: bridge.playPrevious()
                    }

                    Button {
                        text: bridge.isPlaying ? "⏸" : "▶"
                        font.pixelSize: Math.round(24 * Theme.uiScale)
                        Layout.preferredWidth: Math.round(64 * Theme.uiScale)
                        Layout.preferredHeight: Math.round(64 * Theme.uiScale)
                        onClicked: bridge.togglePlayPause()
                    }

                    Button {
                        text: "⏭"
                        font.pixelSize: Math.round(20 * Theme.uiScale)
                        Layout.preferredWidth: Math.round(52 * Theme.uiScale)
                        Layout.preferredHeight: Math.round(52 * Theme.uiScale)
                        onClicked: bridge.playNext()
                    }
                }
            }
        }

        // Right Side: Upcoming Tracks / Party Queue
        Rectangle {
            Layout.preferredWidth: Math.min(Math.round(380 * Theme.uiScale), jukeboxRoot.width * 0.45)
            Layout.fillHeight: true
            color: Theme.surface
            border.color: Theme.panelBorder
            radius: Math.round(8 * Theme.uiScale)

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Math.round(16 * Theme.uiScale)
                spacing: Math.round(12 * Theme.uiScale)

                Text {
                    text: qsTr("Up Next in Jukebox")
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeLarge
                    font.bold: true
                }

                ListView {
                    id: jukeQueue
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: bridge.queueTrackModel
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                    delegate: Rectangle {
                        required property var model
                        width: jukeQueue.width
                        height: Math.round(44 * Theme.uiScale)
                        color: qHover.hovered ? Qt.rgba(1, 1, 1, 0.05) : "transparent"
                        radius: Math.round(4 * Theme.uiScale)

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: Math.round(8 * Theme.uiScale)
                            spacing: Math.round(10 * Theme.uiScale)

                            Text {
                                text: (index + 1)
                                color: Theme.accent
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                Layout.preferredWidth: Math.round(24 * Theme.uiScale)
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2

                                Text {
                                    text: model.title
                                    color: Theme.textPrimary
                                    font.pixelSize: Theme.fontSizeSmall
                                    font.weight: Font.DemiBold
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }

                                Text {
                                    text: model.artist
                                    color: Theme.textSecondary
                                    font.pixelSize: Math.round(10 * Theme.uiScale)
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                            }

                            Text {
                                text: model.durationStr || ""
                                color: Theme.textSecondary
                                font.pixelSize: Math.round(11 * Theme.uiScale)
                            }
                        }

                        HoverHandler { id: qHover; cursorShape: Qt.PointingHandCursor }
                    }
                }
            }
        }
    }
}
