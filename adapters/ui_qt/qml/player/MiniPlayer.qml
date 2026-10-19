import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

Rectangle {
    id: root

    signal requestRestore()
    signal requestContextMenu()

    color: Theme.background
    clip: true

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        onClicked: (mouse) => {
            if (mouse.button === Qt.RightButton) {
                root.requestContextMenu();
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 12

        // Mini Album Art
        Rectangle {
            Layout.preferredWidth: 84
            Layout.preferredHeight: 84
            Layout.alignment: Qt.AlignVCenter
            color: Theme.surface
            border.color: Theme.panelBorder
            radius: Theme.cornerRadiusSmall
            clip: true

            Image {
                anchors.fill: parent
                source: bridge.currentArtUrl
                fillMode: Image.PreserveAspectCrop
            }
        }

        // Info and Controls
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignVCenter
            spacing: 4

            // Top: Track Info & Restore Window Button
            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    Text {
                        text: bridge.currentTrackTitle
                        font.pixelSize: Theme.fontSizeBase
                        font.bold: true
                        color: Theme.textPrimary
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    Text {
                        text: bridge.currentArtist + (bridge.currentAlbum.length > 0 ? " — " + bridge.currentAlbum : "")
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.textSecondary
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }

                Text {
                    text: bridge.currentCodec
                    visible: text.length > 0
                    font.pixelSize: 10
                    font.weight: Font.Medium
                    color: Theme.textMuted
                }

                // Restore Normal Window Button
                Rectangle {
                    width: 24
                    height: 24
                    radius: Theme.cornerRadiusSmall
                    color: restoreMouse.containsMouse ? Theme.surfaceElevated : "transparent"
                    border.color: restoreMouse.containsMouse ? Theme.panelBorder : "transparent"

                    VectorIcon {
                        anchors.centerIn: parent
                        name: "external"
                        width: 11
                        height: 11
                        color: restoreMouse.containsMouse ? Theme.accent : Theme.textMuted
                    }

                    MouseArea {
                        id: restoreMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.requestRestore()
                    }
                    ToolTip.visible: restoreMouse.containsMouse
                    ToolTip.text: "Restore Full Player (Ctrl+Shift+M)"
                    ToolTip.delay: 400
                }
            }

            // Seekbar Row
            RowLayout {
                Layout.fillWidth: true
                spacing: 6

                Text {
                    text: bridge.positionStr
                    font.pixelSize: 9
                    color: Theme.textMuted
                }

                Slider {
                    id: miniSeekSlider
                    Layout.fillWidth: true
                    from: 0
                    to: Math.max(1, bridge.durationMs)
                    value: bridge.positionMs
                    onMoved: bridge.seek(value)
                }

                Text {
                    text: bridge.durationStr
                    font.pixelSize: 9
                    color: Theme.textMuted
                }
            }

            // Transport Deck Row
            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                // Previous Track Button
                Rectangle {
                    width: 26
                    height: 26
                    radius: 13
                    color: miniPrevMouse.containsMouse ? Theme.surfaceElevated : "transparent"
                    VectorIcon { anchors.centerIn: parent; name: "previous"; width: 11; height: 11; color: Theme.textPrimary }
                    MouseArea {
                        id: miniPrevMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: bridge.previousTrack()
                    }
                }

                // Play / Pause Button
                Rectangle {
                    width: 28
                    height: 28
                    radius: 14
                    color: miniPlayMouse.containsMouse ? Theme.accentHover : Theme.accent
                    VectorIcon {
                        anchors.centerIn: parent
                        name: bridge.isPlaying ? "pause" : "play"
                        width: 12
                        height: 12
                        color: Theme.textPrimary
                    }
                    MouseArea {
                        id: miniPlayMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: bridge.togglePlayPause()
                    }
                }

                // Next Track Button
                Rectangle {
                    width: 26
                    height: 26
                    radius: 13
                    color: miniNextMouse.containsMouse ? Theme.surfaceElevated : "transparent"
                    VectorIcon { anchors.centerIn: parent; name: "next"; width: 11; height: 11; color: Theme.textPrimary }
                    MouseArea {
                        id: miniNextMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: bridge.nextTrack()
                    }
                }

                Item { Layout.fillWidth: true }

                // Volume Icon & Slider
                VectorIcon {
                    name: bridge.isMuted ? "volume_mute" : "volume"
                    width: 12
                    height: 12
                    color: Theme.textSecondary
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: bridge.toggleMute()
                    }
                }

                Slider {
                    id: miniVolSlider
                    Layout.preferredWidth: 70
                    from: 0.0
                    to: 1.0
                    value: bridge.volume
                    onMoved: bridge.setVolume(value)
                }
            }
        }
    }
}
