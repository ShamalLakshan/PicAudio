import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

Rectangle {
    id: root

    signal requestSwitchToNowPlaying()
    signal requestToggleFullscreen()
    signal requestToggleEqualizer()
    signal requestSeekBarContextMenu()
    signal requestCodecContextMenu()
    signal requestVolumeContextMenu()

    property bool showEqualizer: false

    Layout.fillWidth: true
    Layout.preferredHeight: 74
    color: Theme.surface

    Item {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16

        // Center: Transport Controls & Scrubber Timeline (Strictly centered on parent window)
        ColumnLayout {
            id: centerTransportControls
            z: 2
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            width: Math.min(540, Math.max(260, parent.width * 0.38))
            spacing: 2

            // Control Buttons Container (Play/Pause strictly centered at 50% width)
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 36

                // Play / Pause Primary Action Button (Strictly centered on column and window)
                Rectangle {
                    id: playPauseBtn
                    anchors.centerIn: parent
                    width: 36
                    height: 36
                    radius: 18
                    color: playPauseMouse.containsMouse ? Theme.accentHover : Theme.accent
                    border.color: playPauseBtn.activeFocus ? Theme.textPrimary : "transparent"
                    border.width: playPauseBtn.activeFocus ? 2 : 0
                    activeFocusOnTab: true
                    Accessible.role: Accessible.Button
                    Accessible.name: bridge.isPlaying ? "Pause playback" : "Start playback"

                    Keys.onSpacePressed: bridge.togglePlayPause()
                    Keys.onReturnPressed: bridge.togglePlayPause()

                    VectorIcon {
                        anchors.centerIn: parent
                        anchors.horizontalCenterOffset: bridge.isPlaying ? 0 : 1
                        name: bridge.isPlaying ? "pause" : "play"
                        width: 16
                        height: 16
                        color: Theme.textPrimary
                    }

                    MouseArea {
                        id: playPauseMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            playPauseBtn.forceActiveFocus();
                            bridge.togglePlayPause();
                        }
                    }

                    ToolTip.visible: playPauseMouse.containsMouse || playPauseBtn.activeFocus
                    ToolTip.text: (bridge.isPlaying ? "Pause" : "Play") + " (" + bridge.getHotkeySequence("play_pause") + ")"
                }

                // Left buttons: Shuffle, Stop, Previous (Adjacent to Play)
                RowLayout {
                    anchors.right: playPauseBtn.left
                    anchors.rightMargin: 16
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 12

                    // Shuffle Button (Cycle: Off -> Tracks -> Albums)
                    Rectangle {
                        id: shuffleBtn
                        width: 24
                        height: 24
                        radius: 2
                        color: bridge.shuffleMode !== 0 ? Theme.selection : (shufMouse.containsMouse ? Theme.surfaceElevated : "transparent")
                        border.color: shuffleBtn.activeFocus ? Theme.accentHover : (bridge.shuffleMode !== 0 ? Theme.accent : "transparent")
                        border.width: shuffleBtn.activeFocus ? 2 : 1
                        activeFocusOnTab: true
                        Accessible.role: Accessible.Button
                        Accessible.name: "Cycle shuffle mode"

                        Keys.onSpacePressed: bridge.cycleShuffleMode()
                        Keys.onReturnPressed: bridge.cycleShuffleMode()

                        VectorIcon {
                            anchors.centerIn: parent
                            name: "shuffle"
                            width: 13
                            height: 13
                            color: bridge.shuffleMode === 1 ? Theme.accent : (bridge.shuffleMode === 2 ? Theme.accentHover : Theme.textMuted)
                        }
                        MouseArea {
                            id: shufMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                shuffleBtn.forceActiveFocus();
                                bridge.cycleShuffleMode();
                            }
                        }
                        ToolTip.visible: shufMouse.containsMouse || shuffleBtn.activeFocus
                        ToolTip.text: (bridge.shuffleMode === 0 ? "Shuffle: Off" : (bridge.shuffleMode === 1 ? "Shuffle: Tracks" : "Shuffle: Albums")) + " (" + bridge.getHotkeySequence("cycle_shuffle") + ")"
                    }

                    // Stop Button
                    Rectangle {
                        id: stopBtn
                        width: 28
                        height: 28
                        radius: 2
                        color: stopMouse.containsMouse ? Theme.surfaceElevated : "transparent"
                        border.color: stopBtn.activeFocus ? Theme.accent : "transparent"
                        border.width: stopBtn.activeFocus ? 2 : 0
                        activeFocusOnTab: true
                        Accessible.role: Accessible.Button
                        Accessible.name: "Stop playback"

                        Keys.onSpacePressed: bridge.stop()
                        Keys.onReturnPressed: bridge.stop()

                        VectorIcon { anchors.centerIn: parent; name: "stop"; width: 12; height: 12; color: Theme.textSecondary }
                        MouseArea {
                            id: stopMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                stopBtn.forceActiveFocus();
                                bridge.stop();
                            }
                        }
                        ToolTip.visible: stopMouse.containsMouse || stopBtn.activeFocus
                        ToolTip.text: "Stop (" + bridge.getHotkeySequence("stop") + ")"
                    }

                    // Previous Button
                    Rectangle {
                        id: prevBtn
                        width: 28
                        height: 28
                        radius: 2
                        color: prevMouse.containsMouse ? Theme.surfaceElevated : "transparent"
                        border.color: prevBtn.activeFocus ? Theme.accent : "transparent"
                        border.width: prevBtn.activeFocus ? 2 : 0
                        activeFocusOnTab: true
                        Accessible.role: Accessible.Button
                        Accessible.name: "Previous track"

                        Keys.onSpacePressed: bridge.previousTrack()
                        Keys.onReturnPressed: bridge.previousTrack()

                        VectorIcon { anchors.centerIn: parent; name: "previous"; width: 14; height: 14; color: Theme.textPrimary }
                        MouseArea {
                            id: prevMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                prevBtn.forceActiveFocus();
                                bridge.previousTrack();
                            }
                        }
                        ToolTip.visible: prevMouse.containsMouse || prevBtn.activeFocus
                        ToolTip.text: "Previous (" + bridge.getHotkeySequence("prev_track") + ")"
                    }
                }

                // Right buttons: Next (Adjacent to Play), Repeat
                RowLayout {
                    anchors.left: playPauseBtn.right
                    anchors.leftMargin: 16
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 12

                    // Next Button
                    Rectangle {
                        id: nextBtn
                        width: 28
                        height: 28
                        radius: 2
                        color: nextMouse.containsMouse ? Theme.surfaceElevated : "transparent"
                        border.color: nextBtn.activeFocus ? Theme.accent : "transparent"
                        border.width: nextBtn.activeFocus ? 2 : 0
                        activeFocusOnTab: true
                        Accessible.role: Accessible.Button
                        Accessible.name: "Next track"

                        Keys.onSpacePressed: bridge.nextTrack()
                        Keys.onReturnPressed: bridge.nextTrack()

                        VectorIcon { anchors.centerIn: parent; name: "next"; width: 14; height: 14; color: Theme.textPrimary }
                        MouseArea {
                            id: nextMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                nextBtn.forceActiveFocus();
                                bridge.nextTrack();
                            }
                        }
                        ToolTip.visible: nextMouse.containsMouse || nextBtn.activeFocus
                        ToolTip.text: "Next (" + bridge.getHotkeySequence("next_track") + ")"
                    }

                    // Repeat Button (Cycle: Off -> All -> One)
                    Rectangle {
                        id: repeatBtn
                        width: 24
                        height: 24
                        radius: 2
                        color: bridge.repeatMode !== 0 ? Theme.selection : (repMouse.containsMouse ? Theme.surfaceElevated : "transparent")
                        border.color: repeatBtn.activeFocus ? Theme.accentHover : (bridge.repeatMode !== 0 ? Theme.accent : "transparent")
                        border.width: repeatBtn.activeFocus ? 2 : 1
                        activeFocusOnTab: true
                        Accessible.role: Accessible.Button
                        Accessible.name: "Cycle repeat mode"

                        Keys.onSpacePressed: bridge.cycleRepeatMode()
                        Keys.onReturnPressed: bridge.cycleRepeatMode()

                        VectorIcon {
                            anchors.centerIn: parent
                            name: bridge.repeatMode === 2 ? "repeat_one" : "repeat"
                            width: 13
                            height: 13
                            color: bridge.repeatMode !== 0 ? Theme.accent : Theme.textMuted
                        }
                        MouseArea {
                            id: repMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                repeatBtn.forceActiveFocus();
                                bridge.cycleRepeatMode();
                            }
                        }
                        ToolTip.visible: repMouse.containsMouse || repeatBtn.activeFocus
                        ToolTip.text: (bridge.repeatMode === 0 ? "Repeat: Off" : (bridge.repeatMode === 1 ? "Repeat: All" : "Repeat: One")) + " (" + bridge.getHotkeySequence("cycle_repeat") + ")"
                    }
                }
            }

            // Scrubber Timeline Row
            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Text {
                    text: bridge.positionStr
                    font.pixelSize: 10
                    color: Theme.textMuted
                    Layout.preferredWidth: 36
                    horizontalAlignment: Text.AlignRight

                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.RightButton
                        onClicked: (mouse) => {
                            if (mouse.button === Qt.RightButton) {
                                root.requestSeekBarContextMenu();
                            }
                        }
                    }
                }

                Slider {
                    id: timelineSlider
                    Layout.fillWidth: true
                    from: 0
                    to: Math.max(1, bridge.durationMs)
                    value: bridge.positionMs
                    onMoved: {
                        bridge.seek(value)
                    }

                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.RightButton
                        onClicked: (mouse) => {
                            if (mouse.button === Qt.RightButton) {
                                root.requestSeekBarContextMenu();
                            }
                        }
                    }

                    background: Rectangle {
                        x: timelineSlider.leftPadding
                        y: timelineSlider.topPadding + timelineSlider.availableHeight / 2 - 2
                        implicitWidth: 200
                        implicitHeight: 4
                        width: timelineSlider.availableWidth
                        height: 4
                        radius: 2
                        color: Theme.surfaceElevated

                        Rectangle {
                            width: timelineSlider.visualPosition * parent.width
                            height: parent.height
                            color: Theme.accent
                            radius: 2
                        }
                    }

                    handle: Rectangle {
                        x: timelineSlider.leftPadding + timelineSlider.visualPosition * (timelineSlider.availableWidth - width)
                        y: timelineSlider.topPadding + timelineSlider.availableHeight / 2 - height / 2
                        implicitWidth: 10
                        implicitHeight: 10
                        radius: 5
                        color: timelineSlider.pressed ? Theme.textPrimary : Theme.accentHover
                    }
                }

                Text {
                    text: bridge.showRemainingTime ? ("-" + bridge.formatDuration(Math.max(0, bridge.durationMs - bridge.positionMs))) : bridge.durationStr
                    font.pixelSize: 10
                    color: Theme.textMuted
                    Layout.preferredWidth: 36

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        onClicked: (mouse) => {
                            if (mouse.button === Qt.RightButton) {
                                root.requestSeekBarContextMenu();
                            } else {
                                bridge.showRemainingTime = !bridge.showRemainingTime;
                            }
                        }
                        ToolTip.visible: containsMouse
                        ToolTip.text: "Click to toggle remaining time • Right-click for options"
                    }
                }
            }
        }

        // Left: Track preview
        RowLayout {
            id: leftTrackPreview
            anchors.left: parent.left
            anchors.right: centerTransportControls.left
            anchors.rightMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            spacing: 12
            clip: true

            Rectangle {
                width: 48
                height: 48
                Layout.preferredWidth: 48
                Layout.preferredHeight: 48
                color: Theme.background
                border.color: Theme.panelBorder
                border.width: 1
                clip: true

                Image {
                    anchors.fill: parent
                    source: bridge.currentArtUrl
                    fillMode: Image.PreserveAspectCrop
                }

                MouseArea {
                    id: deckArtMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.requestSwitchToNowPlaying()
                }
                ToolTip.visible: deckArtMouse.containsMouse
                ToolTip.text: "Open Now Playing View"
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Item {
                    Layout.fillWidth: true
                    implicitHeight: deckTrackCol.implicitHeight

                    ColumnLayout {
                        id: deckTrackCol
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 2

                        Text {
                            text: bridge.currentTrackTitle
                            font.pixelSize: 12
                            font.bold: true
                            color: deckTrackMouse.containsMouse ? Theme.accent : Theme.textPrimary
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        Text {
                            text: bridge.currentArtist + (bridge.currentAlbum.length > 0 ? " • " + bridge.currentAlbum : "")
                            font.pixelSize: 10
                            color: Theme.textSecondary
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }

                    MouseArea {
                        id: deckTrackMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.requestSwitchToNowPlaying()
                    }
                    ToolTip.visible: deckTrackMouse.containsMouse
                    ToolTip.text: "Open Now Playing View"
                }

                RowLayout {
                    spacing: 6
                    Layout.fillWidth: true

                    Rectangle {
                        visible: bridge.currentCodec.length > 0
                        width: codecBadgeText.implicitWidth + 8
                        height: 16
                        radius: 2
                        color: codecMouse.containsMouse ? Theme.surfaceElevated : Theme.selection
                        border.color: Theme.panelBorder
                        border.width: 1

                        Text {
                            id: codecBadgeText
                            anchors.centerIn: parent
                            text: bridge.currentCodec.toUpperCase()
                            font.pixelSize: 8
                            font.bold: true
                            color: Theme.accent
                        }

                        MouseArea {
                            id: codecMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            acceptedButtons: Qt.LeftButton | Qt.RightButton
                            onClicked: (mouse) => {
                                root.requestCodecContextMenu();
                            }
                            ToolTip.visible: codecMouse.containsMouse
                            ToolTip.text: "Right-click or click for Audio Diagnostics & Settings"
                        }
                    }

                    Text {
                        text: bridge.currentAudioSpecs
                        visible: text.length > 0
                        font.pixelSize: 9
                        color: Theme.textMuted
                        elide: Text.ElideRight
                        Layout.fillWidth: true

                        MouseArea {
                            anchors.fill: parent
                            acceptedButtons: Qt.RightButton
                            onClicked: (mouse) => {
                                if (mouse.button === Qt.RightButton) {
                                    root.requestCodecContextMenu();
                                }
                            }
                        }
                    }
                }
            }
        }

        // Right: Volume & Tool Toggles
        RowLayout {
            id: rightControls
            anchors.right: parent.right
            anchors.left: centerTransportControls.right
            anchors.leftMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            spacing: 10
            clip: true

            Rectangle {
                id: muteBtn
                width: 24
                height: 24
                radius: Theme.cornerRadiusSmall
                color: muteMouse.containsMouse ? Theme.surfaceElevated : "transparent"
                border.color: muteBtn.activeFocus ? Theme.accent : "transparent"
                border.width: muteBtn.activeFocus ? 2 : 0
                activeFocusOnTab: true
                Accessible.role: Accessible.Button
                Accessible.name: bridge.isMuted ? "Unmute audio" : "Mute audio"

                Keys.onSpacePressed: bridge.toggleMute()
                Keys.onReturnPressed: bridge.toggleMute()

                VectorIcon {
                    anchors.centerIn: parent
                    name: bridge.isMuted ? "volume_mute" : "volume"
                    width: 14
                    height: 14
                    color: muteBtn.activeFocus ? Theme.accent : Theme.textSecondary
                }

                MouseArea {
                    id: muteMouse
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    onClicked: (mouse) => {
                        muteBtn.forceActiveFocus();
                        if (mouse.button === Qt.RightButton) {
                            root.requestVolumeContextMenu();
                        } else {
                            bridge.toggleMute();
                        }
                    }
                }

                ToolTip.visible: muteMouse.containsMouse || muteBtn.activeFocus
                ToolTip.text: (bridge.isMuted ? "Unmute" : "Mute") + " (" + bridge.getHotkeySequence("toggle_mute") + ")"
            }

            Slider {
                id: volumeSlider
                enabled: !bridge.volumeLocked
                Layout.preferredWidth: (root.width < 750) ? 60 : 80
                from: 0
                to: 1
                value: bridge.volume
                activeFocusOnTab: true
                Accessible.role: Accessible.Slider
                Accessible.name: "Volume level slider"

                Keys.onLeftPressed: bridge.setVolume(bridge.volume - 0.05)
                Keys.onRightPressed: bridge.setVolume(bridge.volume + 0.05)
                Keys.onDownPressed: bridge.setVolume(bridge.volume - 0.05)
                Keys.onUpPressed: bridge.setVolume(bridge.volume + 0.05)

                onMoved: bridge.setVolume(value)

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.RightButton
                    onClicked: (mouse) => {
                        if (mouse.button === Qt.RightButton) {
                            root.requestVolumeContextMenu();
                        }
                    }
                }

                background: Rectangle {
                    x: volumeSlider.leftPadding
                    y: volumeSlider.topPadding + volumeSlider.availableHeight / 2 - 2
                    implicitWidth: 80
                    implicitHeight: 4
                    width: volumeSlider.availableWidth
                    height: 4
                    radius: 2
                    color: Theme.surfaceElevated
                    border.color: volumeSlider.activeFocus ? Theme.accent : "transparent"
                    border.width: volumeSlider.activeFocus ? 1 : 0

                    Rectangle {
                        width: volumeSlider.visualPosition * parent.width
                        height: parent.height
                        color: Theme.accent
                        radius: 2
                    }
                }

                handle: Rectangle {
                    x: volumeSlider.leftPadding + volumeSlider.visualPosition * (volumeSlider.availableWidth - width)
                    y: volumeSlider.topPadding + volumeSlider.availableHeight / 2 - height / 2
                    implicitWidth: volumeSlider.activeFocus ? 10 : 8
                    implicitHeight: volumeSlider.activeFocus ? 10 : 8
                    radius: 5
                    color: volumeSlider.activeFocus ? Theme.accentHover : Theme.textPrimary
                    border.color: volumeSlider.activeFocus ? Theme.textPrimary : "transparent"
                    border.width: volumeSlider.activeFocus ? 1 : 0
                }
            }

            Text {
                text: bridge.volumeLocked ? "🔒 " + Math.round(bridge.volume * 100) + "%" : (Math.round(bridge.volume * 100) + "%")
                font.pixelSize: 9
                color: bridge.volumeLocked ? Theme.accent : Theme.textMuted
                Layout.preferredWidth: 32

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    acceptedButtons: Qt.RightButton
                    onClicked: (mouse) => {
                        if (mouse.button === Qt.RightButton) {
                            root.requestVolumeContextMenu();
                        }
                    }
                    ToolTip.visible: containsMouse
                    ToolTip.text: bridge.volumeLocked ? "Volume is locked (Right-click to unlock)" : "Right-click for volume options"
                }
            }

            Rectangle { width: 1; height: 16; color: Theme.panelBorder }

            // Parametric DSP Equalizer Toggle
            Rectangle {
                id: eqBtn
                width: 24
                height: 24
                radius: 2
                color: (root.showEqualizer || eqMouse.containsMouse) ? Theme.surfaceElevated : "transparent"
                border.color: eqBtn.activeFocus ? Theme.accent : "transparent"
                border.width: eqBtn.activeFocus ? 2 : 0
                activeFocusOnTab: true
                Accessible.role: Accessible.Button
                Accessible.name: "Toggle Parametric Equalizer"

                VectorIcon {
                    anchors.centerIn: parent
                    name: "equalizer"
                    width: 14
                    height: 14
                    color: (root.showEqualizer || eqBtn.activeFocus) ? Theme.accent : Theme.textSecondary
                }

                MouseArea {
                    id: eqMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        eqBtn.forceActiveFocus();
                        root.requestToggleEqualizer();
                    }
                }

                ToolTip.visible: eqMouse.containsMouse || eqBtn.activeFocus
                ToolTip.text: (root.showEqualizer ? "Hide" : "Show") + " Parametric Equalizer (Ctrl+5 / Ctrl+Shift+E)"
            }

            // Fullscreen Toggle
            Rectangle {
                id: fsBtn
                width: 24
                height: 24
                radius: 2
                color: fsMouse.containsMouse ? Theme.surfaceElevated : "transparent"
                border.color: fsBtn.activeFocus ? Theme.accent : "transparent"
                border.width: fsBtn.activeFocus ? 2 : 0
                activeFocusOnTab: true
                Accessible.role: Accessible.Button
                Accessible.name: "Toggle fullscreen window"

                Keys.onSpacePressed: root.requestToggleFullscreen()
                Keys.onReturnPressed: root.requestToggleFullscreen()

                VectorIcon { anchors.centerIn: parent; name: "fullscreen"; width: 12; height: 12; color: fsBtn.activeFocus ? Theme.accent : Theme.textSecondary }
                MouseArea {
                    id: fsMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        fsBtn.forceActiveFocus();
                        root.requestToggleFullscreen();
                    }
                }
                ToolTip.visible: fsMouse.containsMouse || fsBtn.activeFocus
                ToolTip.text: "Toggle Fullscreen (" + bridge.getHotkeySequence("toggle_fullscreen") + ")"
            }
        }
    }
}
