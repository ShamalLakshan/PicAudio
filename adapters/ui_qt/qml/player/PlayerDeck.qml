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
    signal requestSleepTimer()
    signal requestDiagnostics()

    property bool showEqualizer: false

    Layout.fillWidth: true
    Layout.preferredHeight: 74
    color: Theme.surface

    Item {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16

        // Context Menus for Bottom Bar Controls
        Menu {
            id: shuffleMenu
            title: "Shuffle Mode"
            MenuItem {
                text: "Shuffle Off"
                checkable: true
                checked: bridge.shuffleMode === 0
                onTriggered: bridge.setShuffleMode(0)
            }
            MenuItem {
                text: "Shuffle Tracks"
                checkable: true
                checked: bridge.shuffleMode === 1
                onTriggered: bridge.setShuffleMode(1)
            }
            MenuItem {
                text: "Shuffle Albums"
                checkable: true
                checked: bridge.shuffleMode === 2
                onTriggered: bridge.setShuffleMode(2)
            }
        }

        Menu {
            id: repeatMenu
            title: "Repeat Mode"
            MenuItem {
                text: "Repeat Off"
                checkable: true
                checked: bridge.repeatMode === 0
                onTriggered: bridge.setRepeatMode(0)
            }
            MenuItem {
                text: "Repeat All Tracks"
                checkable: true
                checked: bridge.repeatMode === 1
                onTriggered: bridge.setRepeatMode(1)
            }
            MenuItem {
                text: "Repeat One Track"
                checkable: true
                checked: bridge.repeatMode === 2
                onTriggered: bridge.setRepeatMode(2)
            }
        }

        Menu {
            id: seekBackMenu
            title: "Seek Backward"
            MenuItem {
                text: "Seek Backward 5s (Left)"
                onTriggered: bridge.seekRelative(-5000)
            }
            MenuItem {
                text: "Seek Backward 10s"
                onTriggered: bridge.seekRelative(-10000)
            }
            MenuItem {
                text: "Seek Backward 30s (Shift+Left)"
                onTriggered: bridge.seekRelative(-30000)
            }
            MenuSeparator {}
            MenuItem {
                text: "Seek to Track Beginning (Home)"
                onTriggered: bridge.seek(0)
            }
        }

        Menu {
            id: seekFwdMenu
            title: "Seek Forward"
            MenuItem {
                text: "Seek Forward 5s (Right)"
                onTriggered: bridge.seekRelative(5000)
            }
            MenuItem {
                text: "Seek Forward 10s"
                onTriggered: bridge.seekRelative(10000)
            }
            MenuItem {
                text: "Seek Forward 30s (Shift+Right)"
                onTriggered: bridge.seekRelative(30000)
            }
            MenuSeparator {}
            MenuItem {
                text: "Seek to Track End (End)"
                onTriggered: bridge.seek(bridge.durationMs)
            }
        }

        Menu {
            id: speedMenu
            title: "Playback Speed"
            MenuItem {
                text: "0.5x"
                checkable: true
                checked: Math.abs(bridge.playbackRate - 0.5) < 0.05
                onTriggered: bridge.setPlaybackRate(0.5)
            }
            MenuItem {
                text: "0.75x"
                checkable: true
                checked: Math.abs(bridge.playbackRate - 0.75) < 0.05
                onTriggered: bridge.setPlaybackRate(0.75)
            }
            MenuItem {
                text: "0.9x"
                checkable: true
                checked: Math.abs(bridge.playbackRate - 0.9) < 0.05
                onTriggered: bridge.setPlaybackRate(0.9)
            }
            MenuItem {
                text: "1.0x (Normal Speed)"
                checkable: true
                checked: Math.abs(bridge.playbackRate - 1.0) < 0.05
                onTriggered: bridge.setPlaybackRate(1.0)
            }
            MenuItem {
                text: "1.1x"
                checkable: true
                checked: Math.abs(bridge.playbackRate - 1.1) < 0.05
                onTriggered: bridge.setPlaybackRate(1.1)
            }
            MenuItem {
                text: "1.25x"
                checkable: true
                checked: Math.abs(bridge.playbackRate - 1.25) < 0.05
                onTriggered: bridge.setPlaybackRate(1.25)
            }
            MenuItem {
                text: "1.5x"
                checkable: true
                checked: Math.abs(bridge.playbackRate - 1.5) < 0.05
                onTriggered: bridge.setPlaybackRate(1.5)
            }
            MenuItem {
                text: "1.75x"
                checkable: true
                checked: Math.abs(bridge.playbackRate - 1.75) < 0.05
                onTriggered: bridge.setPlaybackRate(1.75)
            }
            MenuItem {
                text: "2.0x"
                checkable: true
                checked: Math.abs(bridge.playbackRate - 2.0) < 0.05
                onTriggered: bridge.setPlaybackRate(2.0)
            }
            MenuSeparator {}
            MenuItem {
                text: "Reset to 1.0x"
                onTriggered: bridge.resetPlaybackRate()
            }
        }

        Menu {
            id: sleepTimerMenu
            title: "Sleep Timer"
            MenuItem {
                text: "15 Minutes"
                onTriggered: bridge.startSleepTimer(15)
            }
            MenuItem {
                text: "30 Minutes"
                onTriggered: bridge.startSleepTimer(30)
            }
            MenuItem {
                text: "45 Minutes"
                onTriggered: bridge.startSleepTimer(45)
            }
            MenuItem {
                text: "60 Minutes"
                onTriggered: bridge.startSleepTimer(60)
            }
            MenuSeparator {}
            MenuItem {
                text: "Cancel Sleep Timer"
                enabled: bridge.isSleepTimerActive
                onTriggered: bridge.cancelSleepTimer()
            }
            MenuItem {
                text: "Configure Sleep Timer..."
                onTriggered: root.requestSleepTimer()
            }
        }

        Menu {
            id: loopMenu
            title: "A-B Looping"
            MenuItem {
                text: "Set Loop Point A (" + (bridge.loopPointA >= 0 ? bridge.formatDuration(bridge.loopPointA) : "--:--") + ")"
                onTriggered: bridge.setLoopPointA()
            }
            MenuItem {
                text: "Set Loop Point B (" + (bridge.loopPointB >= 0 ? bridge.formatDuration(bridge.loopPointB) : "--:--") + ")"
                onTriggered: bridge.setLoopPointB()
            }
            MenuItem {
                text: "Clear Loop"
                enabled: bridge.isLoopActive || bridge.loopPointA >= 0 || bridge.loopPointB >= 0
                onTriggered: bridge.clearLoop()
            }
        }

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

                // Left buttons: Shuffle, Stop After Track, Seek Backward, Previous (Adjacent to Play)
                RowLayout {
                    anchors.right: playPauseBtn.left
                    anchors.rightMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 8

                    // Shuffle Button (Cycle: Off -> Tracks -> Albums)
                    Rectangle {
                        id: shuffleBtn
                        Layout.preferredWidth: 26
                        Layout.preferredHeight: 26
                        Layout.alignment: Qt.AlignVCenter
                        width: 26
                        height: 26
                        radius: Theme.cornerRadiusSmall
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
                            acceptedButtons: Qt.LeftButton | Qt.RightButton
                            onClicked: (mouse) => {
                                shuffleBtn.forceActiveFocus();
                                if (mouse.button === Qt.RightButton) {
                                    shuffleMenu.popup();
                                } else {
                                    bridge.cycleShuffleMode();
                                }
                            }
                        }
                        ToolTip.visible: shufMouse.containsMouse || shuffleBtn.activeFocus
                        ToolTip.text: (bridge.shuffleMode === 0 ? "Shuffle: Off" : (bridge.shuffleMode === 1 ? "Shuffle: Tracks" : "Shuffle: Albums")) + " (" + bridge.getHotkeySequence("cycle_shuffle") + ") • Right-click for options"
                    }

                    // Stop After Current Track Button
                    Rectangle {
                        id: stopAfterBtn
                        Layout.preferredWidth: 26
                        Layout.preferredHeight: 26
                        Layout.alignment: Qt.AlignVCenter
                        width: 26
                        height: 26
                        radius: Theme.cornerRadiusSmall
                        color: bridge.stopAfterCurrentTrack ? Theme.selection : (stopAfterMouse.containsMouse ? Theme.surfaceElevated : "transparent")
                        border.color: stopAfterBtn.activeFocus ? Theme.accentHover : (bridge.stopAfterCurrentTrack ? Theme.accent : "transparent")
                        border.width: stopAfterBtn.activeFocus ? 2 : (bridge.stopAfterCurrentTrack ? 1 : 0)
                        activeFocusOnTab: true
                        Accessible.role: Accessible.Button
                        Accessible.name: "Stop after current track"

                        Keys.onSpacePressed: bridge.toggleStopAfterCurrentTrack()
                        Keys.onReturnPressed: bridge.toggleStopAfterCurrentTrack()

                        VectorIcon {
                            anchors.centerIn: parent
                            name: "stop_after"
                            width: 13
                            height: 13
                            color: bridge.stopAfterCurrentTrack ? Theme.accent : Theme.textMuted
                        }
                        MouseArea {
                            id: stopAfterMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                stopAfterBtn.forceActiveFocus();
                                bridge.toggleStopAfterCurrentTrack();
                            }
                        }
                        ToolTip.visible: stopAfterMouse.containsMouse || stopAfterBtn.activeFocus
                        ToolTip.text: "Stop After Current Track: " + (bridge.stopAfterCurrentTrack ? "On" : "Off") + " (" + bridge.getHotkeySequence("stop_after_current") + ")"
                    }

                    // Seek Backward Button
                    Rectangle {
                        id: seekBackBtn
                        Layout.preferredWidth: 26
                        Layout.preferredHeight: 26
                        Layout.alignment: Qt.AlignVCenter
                        width: 26
                        height: 26
                        radius: Theme.cornerRadiusSmall
                        color: seekBackMouse.containsMouse ? Theme.surfaceElevated : "transparent"
                        border.color: seekBackBtn.activeFocus ? Theme.accent : "transparent"
                        border.width: seekBackBtn.activeFocus ? 2 : 0
                        activeFocusOnTab: true
                        Accessible.role: Accessible.Button
                        Accessible.name: "Seek backward"

                        Keys.onSpacePressed: bridge.seekRelative(-bridge.shortSeekStepSec * 1000)
                        Keys.onReturnPressed: bridge.seekRelative(-bridge.shortSeekStepSec * 1000)

                        VectorIcon {
                            anchors.centerIn: parent
                            name: "seek_backward"
                            width: 13
                            height: 13
                            color: Theme.textSecondary
                        }
                        MouseArea {
                            id: seekBackMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            acceptedButtons: Qt.LeftButton | Qt.RightButton
                            onClicked: (mouse) => {
                                seekBackBtn.forceActiveFocus();
                                if (mouse.button === Qt.RightButton) {
                                    seekBackMenu.popup();
                                } else {
                                    bridge.seekRelative(-bridge.shortSeekStepSec * 1000);
                                }
                            }
                        }
                        ToolTip.visible: seekBackMouse.containsMouse || seekBackBtn.activeFocus
                        ToolTip.text: "Seek Backward " + bridge.shortSeekStepSec + "s (" + bridge.getHotkeySequence("seek_backward_short") + ") • Right-click for options"
                    }

                    // Previous Button
                    Rectangle {
                        id: prevBtn
                        Layout.preferredWidth: 28
                        Layout.preferredHeight: 28
                        Layout.alignment: Qt.AlignVCenter
                        width: 28
                        height: 28
                        radius: Theme.cornerRadiusSmall
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

                // Right buttons: Next, Seek Forward, Stop, Repeat (Adjacent to Play)
                RowLayout {
                    anchors.left: playPauseBtn.right
                    anchors.leftMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 8

                    // Next Button
                    Rectangle {
                        id: nextBtn
                        Layout.preferredWidth: 28
                        Layout.preferredHeight: 28
                        Layout.alignment: Qt.AlignVCenter
                        width: 28
                        height: 28
                        radius: Theme.cornerRadiusSmall
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

                    // Seek Forward Button
                    Rectangle {
                        id: seekFwdBtn
                        Layout.preferredWidth: 26
                        Layout.preferredHeight: 26
                        Layout.alignment: Qt.AlignVCenter
                        width: 26
                        height: 26
                        radius: Theme.cornerRadiusSmall
                        color: seekFwdMouse.containsMouse ? Theme.surfaceElevated : "transparent"
                        border.color: seekFwdBtn.activeFocus ? Theme.accent : "transparent"
                        border.width: seekFwdBtn.activeFocus ? 2 : 0
                        activeFocusOnTab: true
                        Accessible.role: Accessible.Button
                        Accessible.name: "Seek forward"

                        Keys.onSpacePressed: bridge.seekRelative(bridge.shortSeekStepSec * 1000)
                        Keys.onReturnPressed: bridge.seekRelative(bridge.shortSeekStepSec * 1000)

                        VectorIcon {
                            anchors.centerIn: parent
                            name: "seek_forward"
                            width: 13
                            height: 13
                            color: Theme.textSecondary
                        }
                        MouseArea {
                            id: seekFwdMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            acceptedButtons: Qt.LeftButton | Qt.RightButton
                            onClicked: (mouse) => {
                                seekFwdBtn.forceActiveFocus();
                                if (mouse.button === Qt.RightButton) {
                                    seekFwdMenu.popup();
                                } else {
                                    bridge.seekRelative(bridge.shortSeekStepSec * 1000);
                                }
                            }
                        }
                        ToolTip.visible: seekFwdMouse.containsMouse || seekFwdBtn.activeFocus
                        ToolTip.text: "Seek Forward " + bridge.shortSeekStepSec + "s (" + bridge.getHotkeySequence("seek_forward_short") + ") • Right-click for options"
                    }

                    // Stop Button
                    Rectangle {
                        id: stopBtn
                        Layout.preferredWidth: 28
                        Layout.preferredHeight: 28
                        Layout.alignment: Qt.AlignVCenter
                        width: 28
                        height: 28
                        radius: Theme.cornerRadiusSmall
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

                    // Repeat Button (Cycle: Off -> All -> One)
                    Rectangle {
                        id: repeatBtn
                        Layout.preferredWidth: 26
                        Layout.preferredHeight: 26
                        Layout.alignment: Qt.AlignVCenter
                        width: 26
                        height: 26
                        radius: Theme.cornerRadiusSmall
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
                            acceptedButtons: Qt.LeftButton | Qt.RightButton
                            onClicked: (mouse) => {
                                repeatBtn.forceActiveFocus();
                                if (mouse.button === Qt.RightButton) {
                                    repeatMenu.popup();
                                } else {
                                    bridge.cycleRepeatMode();
                                }
                            }
                        }
                        ToolTip.visible: repMouse.containsMouse || repeatBtn.activeFocus
                        ToolTip.text: (bridge.repeatMode === 0 ? "Repeat: Off" : (bridge.repeatMode === 1 ? "Repeat: All" : "Repeat: One")) + " (" + bridge.getHotkeySequence("cycle_repeat") + ") • Right-click for options"
                    }
                }
            }

            // Scrubber Timeline Row
            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Text {
                    id: positionLabel
                    text: bridge.positionStr
                    font.pixelSize: 10
                    color: Theme.textMuted
                    Layout.preferredWidth: Math.max(44, positionLabel.implicitWidth + 4)
                    Layout.minimumWidth: 44
                    Layout.alignment: Qt.AlignVCenter
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

                    background: Item {
                        x: timelineSlider.leftPadding
                        y: timelineSlider.topPadding + (timelineSlider.availableHeight - height) / 2
                        implicitWidth: 200
                        implicitHeight: bridge.waveformSeekbar ? 20 : 4
                        width: timelineSlider.availableWidth
                        height: bridge.waveformSeekbar ? 20 : 4

                        // Base track bar (when waveform is disabled)
                        Rectangle {
                            anchors.centerIn: parent
                            width: parent.width
                            height: 4
                            radius: 2
                            color: Theme.surfaceElevated
                            visible: !bridge.waveformSeekbar

                            Rectangle {
                                width: timelineSlider.visualPosition * parent.width
                                height: parent.height
                                color: Theme.accent
                                radius: 2
                            }
                        }

                        // Audio peak waveform visualization
                        Canvas {
                            id: waveformCanvas
                            anchors.fill: parent
                            visible: bridge.waveformSeekbar
                            opacity: 0.9

                            Connections {
                                target: bridge
                                function onWaveformPeaksChanged() { waveformCanvas.requestPaint(); }
                                function onPositionChanged() { waveformCanvas.requestPaint(); }
                            }

                            onPaint: {
                                var ctx = getContext("2d");
                                ctx.reset();
                                var w = width;
                                var h = height;
                                var peaks = bridge.waveformPeaks;
                                if (!peaks || peaks.length === 0) {
                                    ctx.fillStyle = Theme.surfaceElevated;
                                    ctx.fillRect(0, h / 2 - 1, w, 2);
                                    return;
                                }

                                var playedW = timelineSlider.visualPosition * w;
                                var barCount = Math.min(peaks.length, Math.floor(w / 3));
                                var step = w / barCount;
                                var barWidth = Math.max(1.5, step - 1.0);

                                for (var i = 0; i < barCount; ++i) {
                                    var peakIdx = Math.floor(i * peaks.length / barCount);
                                    var amp = Math.max(0.1, peaks[peakIdx]);
                                    var barH = Math.max(2, amp * (h - 2));
                                    var xPos = i * step;
                                    var yPos = (h - barH) / 2;

                                    if (xPos + barWidth <= playedW) {
                                        ctx.fillStyle = Theme.accent;
                                    } else {
                                        ctx.fillStyle = Theme.surfaceElevated;
                                    }
                                    ctx.fillRect(xPos, yPos, barWidth, barH);
                                }
                            }
                        }

                        // A-B Looping Range Highlight & Markers
                        Item {
                            anchors.fill: parent
                            visible: bridge.loopPointA >= 0 && bridge.durationMs > 0

                            Rectangle {
                                id: loopRegion
                                visible: bridge.loopPointB > bridge.loopPointA
                                x: (bridge.loopPointA / bridge.durationMs) * parent.width
                                width: Math.max(2, ((bridge.loopPointB - bridge.loopPointA) / bridge.durationMs) * parent.width)
                                height: parent.height
                                color: Theme.accent
                                opacity: 0.25
                                radius: 2
                            }

                            Rectangle {
                                x: (bridge.loopPointA / bridge.durationMs) * parent.width - 1
                                width: 3
                                height: parent.height
                                color: Theme.accent
                                radius: 1
                            }

                            Rectangle {
                                visible: bridge.loopPointB >= 0
                                x: (bridge.loopPointB / bridge.durationMs) * parent.width - 1
                                width: 3
                                height: parent.height
                                color: Theme.accentHover
                                radius: 1
                            }
                        }
                    }

                    handle: Rectangle {
                        x: timelineSlider.leftPadding + timelineSlider.visualPosition * (timelineSlider.availableWidth - width)
                        y: timelineSlider.topPadding + timelineSlider.availableHeight / 2 - height / 2
                        implicitWidth: bridge.waveformSeekbar ? 12 : 10
                        implicitHeight: bridge.waveformSeekbar ? 22 : 10
                        radius: bridge.waveformSeekbar ? 2 : 5
                        color: timelineSlider.pressed ? Theme.textPrimary : Theme.accentHover
                    }
                }

                Text {
                    id: durationLabel
                    text: bridge.showRemainingTime ? ("-" + bridge.formatDuration(Math.max(0, bridge.durationMs - bridge.positionMs))) : bridge.durationStr
                    font.pixelSize: 10
                    color: Theme.textMuted
                    Layout.preferredWidth: Math.max(48, durationLabel.implicitWidth + 4)
                    Layout.minimumWidth: 48
                    Layout.alignment: Qt.AlignVCenter
                    horizontalAlignment: Text.AlignRight

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
                Layout.alignment: Qt.AlignVCenter
                radius: Theme.cornerRadiusSmall
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
                        id: codecBadge
                        visible: bridge.currentCodec.length > 0
                        Layout.preferredWidth: Math.max(36, codecBadgeText.implicitWidth + 12)
                        Layout.preferredHeight: 18
                        Layout.minimumWidth: 36
                        Layout.alignment: Qt.AlignVCenter
                        width: Layout.preferredWidth
                        height: 18
                        radius: Theme.cornerRadiusSmall
                        color: codecMouse.containsMouse ? Theme.surfaceElevated : Theme.selection
                        border.color: Theme.panelBorder
                        border.width: 1
                        clip: true

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
                                if (mouse.button === Qt.RightButton) {
                                    root.requestCodecContextMenu();
                                } else {
                                    root.requestDiagnostics();
                                }
                            }
                            ToolTip.visible: codecMouse.containsMouse
                            ToolTip.text: "Audio Stream: " + bridge.currentCodec.toUpperCase() + " • Audio Pipeline Diagnostics (Ctrl+J) • Right-click for options"
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

        // Right: A-B Loop, Speed, Sleep Timer, Diagnostics, Volume & Tool Toggles
        RowLayout {
            id: rightControls
            anchors.right: parent.right
            anchors.left: centerTransportControls.right
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            spacing: 6
            clip: true

            Item { Layout.fillWidth: true }

            // A-B Looping Point A Button
            Rectangle {
                id: loopPointABtn
                Layout.preferredWidth: 24
                Layout.preferredHeight: 24
                Layout.alignment: Qt.AlignVCenter
                width: 24
                height: 24
                radius: Theme.cornerRadiusSmall
                color: bridge.loopPointA >= 0 ? Theme.selection : (loopAMouse.containsMouse ? Theme.surfaceElevated : "transparent")
                border.color: loopPointABtn.activeFocus ? Theme.accent : (bridge.loopPointA >= 0 ? Theme.accent : Theme.panelBorder)
                border.width: loopPointABtn.activeFocus ? 2 : 1
                activeFocusOnTab: true
                Accessible.role: Accessible.Button
                Accessible.name: "Set Loop Point A"

                Keys.onSpacePressed: bridge.setLoopPointA()
                Keys.onReturnPressed: bridge.setLoopPointA()

                Text {
                    anchors.centerIn: parent
                    text: "A"
                    font.pixelSize: 10
                    font.bold: true
                    color: bridge.loopPointA >= 0 ? Theme.accent : Theme.textSecondary
                }

                MouseArea {
                    id: loopAMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    onClicked: (mouse) => {
                        loopPointABtn.forceActiveFocus();
                        if (mouse.button === Qt.RightButton) {
                            loopMenu.popup();
                        } else {
                            bridge.setLoopPointA();
                        }
                    }
                }

                ToolTip.visible: loopAMouse.containsMouse || loopPointABtn.activeFocus
                ToolTip.text: bridge.loopPointA >= 0 ? ("Loop Point A: " + bridge.formatDuration(bridge.loopPointA) + " • Click to set current (" + bridge.getHotkeySequence("set_loop_a") + ") • Right-click for options") : ("Set Loop Point A (" + bridge.getHotkeySequence("set_loop_a") + ")")
            }

            // A-B Looping Point B Button
            Rectangle {
                id: loopPointBBtn
                Layout.preferredWidth: 24
                Layout.preferredHeight: 24
                Layout.alignment: Qt.AlignVCenter
                width: 24
                height: 24
                radius: Theme.cornerRadiusSmall
                color: bridge.loopPointB >= 0 ? Theme.selection : (loopBMouse.containsMouse ? Theme.surfaceElevated : "transparent")
                border.color: loopPointBBtn.activeFocus ? Theme.accent : (bridge.loopPointB >= 0 ? Theme.accentHover : Theme.panelBorder)
                border.width: loopPointBBtn.activeFocus ? 2 : 1
                activeFocusOnTab: true
                Accessible.role: Accessible.Button
                Accessible.name: "Set Loop Point B"

                Keys.onSpacePressed: bridge.setLoopPointB()
                Keys.onReturnPressed: bridge.setLoopPointB()

                Text {
                    anchors.centerIn: parent
                    text: "B"
                    font.pixelSize: 10
                    font.bold: true
                    color: bridge.loopPointB >= 0 ? Theme.accentHover : Theme.textSecondary
                }

                MouseArea {
                    id: loopBMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    onClicked: (mouse) => {
                        loopPointBBtn.forceActiveFocus();
                        if (mouse.button === Qt.RightButton) {
                            loopMenu.popup();
                        } else {
                            bridge.setLoopPointB();
                        }
                    }
                }

                ToolTip.visible: loopBMouse.containsMouse || loopPointBBtn.activeFocus
                ToolTip.text: bridge.loopPointB >= 0 ? ("Loop Point B: " + bridge.formatDuration(bridge.loopPointB) + " • Click to set current (" + bridge.getHotkeySequence("set_loop_b") + ") • Right-click for options") : ("Set Loop Point B (" + bridge.getHotkeySequence("set_loop_b") + ")")
            }

            // A-B Looping Clear Button (Visible when loop points set)
            Rectangle {
                id: clearLoopBtn
                visible: bridge.loopPointA >= 0 || bridge.loopPointB >= 0
                Layout.preferredWidth: visible ? 20 : 0
                Layout.preferredHeight: 24
                Layout.alignment: Qt.AlignVCenter
                width: 20
                height: 24
                radius: Theme.cornerRadiusSmall
                color: clearLoopMouse.containsMouse ? Theme.surfaceElevated : "transparent"
                border.color: clearLoopBtn.activeFocus ? Theme.accent : "transparent"
                border.width: clearLoopBtn.activeFocus ? 2 : 0
                activeFocusOnTab: true
                Accessible.role: Accessible.Button
                Accessible.name: "Clear loop points"

                Keys.onSpacePressed: bridge.clearLoop()
                Keys.onReturnPressed: bridge.clearLoop()

                VectorIcon {
                    anchors.centerIn: parent
                    name: "clear"
                    width: 9
                    height: 9
                    color: Theme.textMuted
                }

                MouseArea {
                    id: clearLoopMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        clearLoopBtn.forceActiveFocus();
                        bridge.clearLoop();
                    }
                }

                ToolTip.visible: clearLoopMouse.containsMouse || clearLoopBtn.activeFocus
                ToolTip.text: "Clear A-B Loop (" + bridge.getHotkeySequence("clear_loop") + ")"
            }

            // Playback Speed Selector Chip
            Rectangle {
                id: speedBtn
                Layout.preferredWidth: Math.max(36, speedText.implicitWidth + 16)
                Layout.preferredHeight: 24
                Layout.minimumWidth: 36
                Layout.alignment: Qt.AlignVCenter
                width: Layout.preferredWidth
                height: 24
                radius: Theme.cornerRadiusSmall
                color: speedMouse.containsMouse ? Theme.surfaceElevated : (Math.abs(bridge.playbackRate - 1.0) > 0.05 ? Theme.selection : "transparent")
                border.color: speedBtn.activeFocus ? Theme.accent : (Math.abs(bridge.playbackRate - 1.0) > 0.05 ? Theme.accent : Theme.panelBorder)
                border.width: speedBtn.activeFocus ? 2 : 1
                activeFocusOnTab: true
                clip: true
                Accessible.role: Accessible.Button
                Accessible.name: "Playback speed selector"

                Keys.onSpacePressed: bridge.cyclePlaybackRate()
                Keys.onReturnPressed: bridge.cyclePlaybackRate()

                Text {
                    id: speedText
                    anchors.centerIn: parent
                    text: bridge.playbackRate.toFixed(2).replace(/\.00$/, "").replace(/(\.[1-9])0$/, "$1") + "x"
                    font.pixelSize: 10
                    font.bold: true
                    color: Math.abs(bridge.playbackRate - 1.0) > 0.05 ? Theme.accent : Theme.textSecondary
                }

                MouseArea {
                    id: speedMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    onClicked: (mouse) => {
                        speedBtn.forceActiveFocus();
                        if (mouse.button === Qt.RightButton) {
                            speedMenu.popup();
                        } else {
                            bridge.cyclePlaybackRate();
                        }
                    }
                }

                ToolTip.visible: speedMouse.containsMouse || speedBtn.activeFocus
                ToolTip.text: "Playback Speed: " + bridge.playbackRate + "x (Click to cycle • Right-click for presets)"
            }

            // Sleep Timer Quick Access & Status
            Rectangle {
                id: sleepTimerBtn
                Layout.preferredWidth: bridge.isSleepTimerActive ? Math.max(54, sleepRow.implicitWidth + 14) : 26
                Layout.preferredHeight: 24
                Layout.minimumWidth: 26
                Layout.alignment: Qt.AlignVCenter
                width: Layout.preferredWidth
                height: 24
                radius: Theme.cornerRadiusSmall
                color: bridge.isSleepTimerActive ? Theme.selection : (sleepMouse.containsMouse ? Theme.surfaceElevated : "transparent")
                border.color: sleepTimerBtn.activeFocus ? Theme.accent : (bridge.isSleepTimerActive ? Theme.accent : "transparent")
                border.width: sleepTimerBtn.activeFocus ? 2 : (bridge.isSleepTimerActive ? 1 : 0)
                activeFocusOnTab: true
                clip: true
                Accessible.role: Accessible.Button
                Accessible.name: "Playback Sleep Timer"

                Keys.onSpacePressed: root.requestSleepTimer()
                Keys.onReturnPressed: root.requestSleepTimer()

                RowLayout {
                    id: sleepRow
                    anchors.centerIn: parent
                    anchors.leftMargin: 6
                    anchors.rightMargin: 6
                    spacing: 4

                    VectorIcon {
                        name: "timer"
                        width: 12
                        height: 12
                        color: bridge.isSleepTimerActive ? Theme.accent : Theme.textSecondary
                    }

                    Text {
                        visible: bridge.isSleepTimerActive
                        text: Math.ceil(bridge.sleepTimerRemainingSec / 60) + "m"
                        font.pixelSize: 9
                        font.bold: true
                        color: Theme.accent
                    }
                }

                MouseArea {
                    id: sleepMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    onClicked: (mouse) => {
                        sleepTimerBtn.forceActiveFocus();
                        if (mouse.button === Qt.RightButton) {
                            sleepTimerMenu.popup();
                        } else {
                            root.requestSleepTimer();
                        }
                    }
                }

                ToolTip.visible: sleepMouse.containsMouse || sleepTimerBtn.activeFocus
                ToolTip.text: bridge.isSleepTimerActive ? ("Sleep Timer Active: " + Math.ceil(bridge.sleepTimerRemainingSec / 60) + "m remaining • Click to adjust • Right-click for quick actions") : "Playback Sleep Timer... (Click to set)"
            }

            // Audio Pipeline Diagnostics Modal Launcher
            Rectangle {
                id: diagBtn
                visible: root.width >= 900
                Layout.preferredWidth: visible ? 26 : 0
                Layout.preferredHeight: 24
                Layout.minimumWidth: 26
                Layout.alignment: Qt.AlignVCenter
                width: 26
                height: 24
                radius: Theme.cornerRadiusSmall
                color: diagMouse.containsMouse ? Theme.surfaceElevated : "transparent"
                border.color: diagBtn.activeFocus ? Theme.accent : "transparent"
                border.width: diagBtn.activeFocus ? 2 : 0
                activeFocusOnTab: true
                Accessible.role: Accessible.Button
                Accessible.name: "Audio Pipeline Diagnostics"

                Keys.onSpacePressed: root.requestDiagnostics()
                Keys.onReturnPressed: root.requestDiagnostics()

                VectorIcon {
                    anchors.centerIn: parent
                    name: "diagnostics"
                    width: 13
                    height: 13
                    color: Theme.textSecondary
                }

                MouseArea {
                    id: diagMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        diagBtn.forceActiveFocus();
                        root.requestDiagnostics();
                    }
                }

                ToolTip.visible: diagMouse.containsMouse || diagBtn.activeFocus
                ToolTip.text: "Audio Pipeline Diagnostics (Ctrl+J)"
            }

            Rectangle {
                Layout.preferredWidth: 1
                Layout.preferredHeight: 16
                Layout.leftMargin: 2
                Layout.rightMargin: 2
                Layout.alignment: Qt.AlignVCenter
                color: Theme.panelBorder
            }

            // Parametric DSP Equalizer Toggle
            Rectangle {
                id: eqBtn
                Layout.preferredWidth: 26
                Layout.preferredHeight: 24
                Layout.minimumWidth: 26
                Layout.alignment: Qt.AlignVCenter
                width: 26
                height: 24
                radius: Theme.cornerRadiusSmall
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

            // Volume Controls: Mute
            Rectangle {
                id: muteBtn
                Layout.preferredWidth: 26
                Layout.preferredHeight: 24
                Layout.minimumWidth: 26
                Layout.alignment: Qt.AlignVCenter
                width: 26
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

            // Volume Slider
            Slider {
                id: volumeSlider
                enabled: !bridge.volumeLocked
                Layout.preferredWidth: (root.width < 1050) ? 60 : 80
                Layout.alignment: Qt.AlignVCenter
                Layout.leftMargin: 2
                Layout.rightMargin: 2
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
                    implicitWidth: 75
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

            // Volume Percentage & Lock Indicator
            Text {
                text: bridge.volumeLocked ? "🔒 " + Math.round(bridge.volume * 100) + "%" : (Math.round(bridge.volume * 100) + "%")
                font.pixelSize: 9
                color: bridge.volumeLocked ? Theme.accent : Theme.textMuted
                Layout.preferredWidth: 42
                Layout.minimumWidth: 38
                Layout.alignment: Qt.AlignVCenter
                horizontalAlignment: Text.AlignRight

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

            // Fullscreen Toggle
            Rectangle {
                id: fsBtn
                Layout.preferredWidth: 26
                Layout.preferredHeight: 24
                Layout.minimumWidth: 26
                Layout.alignment: Qt.AlignVCenter
                width: 26
                height: 24
                radius: Theme.cornerRadiusSmall
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
