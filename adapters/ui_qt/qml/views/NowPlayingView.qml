import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../components"

Item {
    id: root

    // User preference for lyrics visibility (optional, toggleable)
    property bool showLyrics: false
    property int visualizerStyle: 0 // 0: Fluid Wave, 1: Audio Spectrum Bars
    property int compactTab: 0 // 0: Track & Controls, 1: Lyrics (on very narrow screens)

    readonly property bool isWideLayout: width >= 860
    readonly property bool hasActiveTrack: bridge.currentTrackTitle.length > 0

    // Auto-scroll lyrics when active index changes
    Connections {
        target: bridge
        function onActiveLyricIndexChanged() {
            if (root.showLyrics && bridge.activeLyricIndex >= 0 && lyricsListView.count > bridge.activeLyricIndex) {
                lyricsListView.positionViewAtIndex(bridge.activeLyricIndex, ListView.Center);
            }
        }
    }

    // Base background with subtle ambient studio glow
    Rectangle {
        anchors.fill: parent
        color: Theme.background
        clip: true

        // Soft ambient backlight glow centered in the view
        Rectangle {
            anchors.centerIn: parent
            width: Math.min(parent.width * 0.85, 750)
            height: Math.min(parent.height * 0.85, 650)
            radius: width * 0.5
            color: Theme.accent
            opacity: bridge.isPlaying ? 0.05 : 0.02
            Behavior on opacity { NumberAnimation { duration: 400 } }
        }
    }

    // Empty state when nothing is playing or loaded
    Rectangle {
        anchors.fill: parent
        color: "transparent"
        visible: !root.hasActiveTrack

        ColumnLayout {
            anchors.centerIn: parent
            spacing: Theme.spacingLarge

            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                width: 72
                height: 72
                radius: 36
                color: Theme.surfaceElevated
                border.color: Theme.panelBorder
                border.width: 1

                VectorIcon {
                    anchors.centerIn: parent
                    name: "disc"
                    width: 38
                    height: 38
                    color: Theme.textMuted
                }
            }

            ColumnLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: Theme.spacingSmall

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "No Track Playing"
                    font.pixelSize: Theme.fontSizeTitle + 2
                    font.bold: true
                    color: Theme.textPrimary
                }

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "Select a song from your library or double-click to start playback."
                    font.pixelSize: Theme.fontSizeBase
                    color: Theme.textMuted
                }
            }

            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                width: browseBtnRow.implicitWidth + 24
                height: 34
                radius: Theme.cornerRadiusMedium
                color: browseMouse.containsMouse ? Theme.accentHover : Theme.accent

                RowLayout {
                    id: browseBtnRow
                    anchors.centerIn: parent
                    spacing: 8
                    VectorIcon { name: "table"; width: 12; height: 12; color: Theme.textPrimary }
                    Text {
                        text: "Browse Music Library"
                        font.pixelSize: Theme.fontSizeBase
                        font.bold: true
                        color: Theme.textPrimary
                    }
                }

                MouseArea {
                    id: browseMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: switchViewMode(1)
                }
            }
        }
    }

    // Active Now Playing Layout
    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        visible: root.hasActiveTrack

        // Top Navigation & Action Toolbar
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 42
            color: Theme.surface
            border.color: Theme.panelBorder
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.panelPadding * 1.5
                anchors.rightMargin: Theme.panelPadding * 1.5
                spacing: Theme.spacingMedium

                // Back breadcrumb
                Rectangle {
                    width: backText.implicitWidth + 18
                    height: 26
                    radius: Theme.cornerRadiusSmall
                    color: backMouse.containsMouse ? Theme.selection : "transparent"
                    border.color: Theme.panelBorder

                    RowLayout {
                        anchors.centerIn: parent
                        spacing: 5
                        VectorIcon { name: "previous"; width: 9; height: 9; color: Theme.accent }
                        Text {
                            id: backText
                            text: previousViewMode === 0 ? "Albums" : (previousViewMode === 2 ? "Album Details" : "Tracks")
                            font.pixelSize: Theme.fontSizeSmall
                            font.bold: true
                            color: Theme.textSecondary
                        }
                    }

                    MouseArea {
                        id: backMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: switchViewMode(previousViewMode)
                    }
                    ToolTip.visible: backMouse.containsMouse
                    ToolTip.text: "Return to previous view"
                }

                Item { Layout.fillWidth: true }

                // Center Title Badge
                RowLayout {
                    spacing: 8
                    VectorIcon { name: "disc"; width: 13; height: 13; color: Theme.accent }
                    Text {
                        text: "NOW PLAYING"
                        font.pixelSize: Theme.fontSizeSmall
                        font.bold: true
                        font.letterSpacing: 1.5
                        color: Theme.textPrimary
                    }
                    Rectangle {
                        width: 4
                        height: 4
                        radius: 2
                        color: bridge.isPlaying ? Theme.accent : Theme.textMuted
                    }
                    Text {
                        text: bridge.isPlaying ? "PLAYING" : "PAUSED"
                        font.pixelSize: Theme.fontSizeSmall - 1
                        font.bold: true
                        color: bridge.isPlaying ? Theme.accent : Theme.textMuted
                    }
                }

                Item { Layout.fillWidth: true }

                // Narrow screen tab switcher (Track vs Lyrics)
                RowLayout {
                    visible: !root.isWideLayout && root.showLyrics
                    spacing: 4

                    Rectangle {
                        width: narrowTrackTxt.implicitWidth + 14
                        height: 26
                        radius: Theme.cornerRadiusSmall
                        color: root.compactTab === 0 ? Theme.selection : "transparent"
                        border.color: root.compactTab === 0 ? Theme.accent : Theme.panelBorder
                        Text {
                            id: narrowTrackTxt
                            anchors.centerIn: parent
                            text: "Player"
                            font.pixelSize: Theme.fontSizeSmall
                            font.bold: true
                            color: root.compactTab === 0 ? Theme.textPrimary : Theme.textMuted
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.compactTab = 0
                        }
                    }

                    Rectangle {
                        width: narrowLrcTxt.implicitWidth + 14
                        height: 26
                        radius: Theme.cornerRadiusSmall
                        color: root.compactTab === 1 ? Theme.selection : "transparent"
                        border.color: root.compactTab === 1 ? Theme.accent : Theme.panelBorder
                        Text {
                            id: narrowLrcTxt
                            anchors.centerIn: parent
                            text: "Lyrics"
                            font.pixelSize: Theme.fontSizeSmall
                            font.bold: true
                            color: root.compactTab === 1 ? Theme.textPrimary : Theme.textMuted
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.compactTab = 1
                        }
                    }
                }

                // Optional Lyrics Toggle Button
                Rectangle {
                    width: lyricsPillRow.implicitWidth + 18
                    height: 26
                    radius: Theme.cornerRadiusSmall
                    color: root.showLyrics ? Theme.selection : (lyricsMouse.containsMouse ? Theme.surfaceElevated : "transparent")
                    border.color: root.showLyrics ? Theme.accent : Theme.panelBorder
                    border.width: 1

                    RowLayout {
                        id: lyricsPillRow
                        anchors.centerIn: parent
                        spacing: 6

                        VectorIcon {
                            name: "lyrics"
                            width: 11
                            height: 11
                            color: root.showLyrics ? Theme.accent : (bridge.hasLyrics ? Theme.textPrimary : Theme.textMuted)
                        }

                        Text {
                            text: root.showLyrics ? "Lyrics (On)" : "Lyrics"
                            font.pixelSize: Theme.fontSizeSmall
                            font.bold: root.showLyrics
                            color: root.showLyrics ? Theme.accent : Theme.textSecondary
                        }

                        // Dot indicator if lyrics exist
                        Rectangle {
                            visible: bridge.hasLyrics && !root.showLyrics
                            width: 5
                            height: 5
                            radius: 2.5
                            color: Theme.accent
                        }
                    }

                    MouseArea {
                        id: lyricsMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            root.showLyrics = !root.showLyrics;
                            if (root.showLyrics) {
                                root.compactTab = 1;
                            }
                        }
                    }

                    ToolTip.visible: lyricsMouse.containsMouse
                    ToolTip.text: root.showLyrics ? "Hide Synchronized Lyrics" : (bridge.hasLyrics ? "Show Synchronized Lyrics" : "Toggle Lyrics Panel")
                }

                // Visualizer Style Switcher Pill
                Rectangle {
                    width: vizPillRow.implicitWidth + 16
                    height: 26
                    radius: Theme.cornerRadiusSmall
                    color: vizMouse.containsMouse ? Theme.surfaceElevated : "transparent"
                    border.color: Theme.panelBorder
                    border.width: 1

                    RowLayout {
                        id: vizPillRow
                        anchors.centerIn: parent
                        spacing: 5

                        VectorIcon {
                            name: "wave"
                            width: 11
                            height: 11
                            color: Theme.textSecondary
                        }

                        Text {
                            text: root.visualizerStyle === 0 ? "Wave" : "Bars"
                            font.pixelSize: Theme.fontSizeSmall
                            color: Theme.textSecondary
                        }
                    }

                    MouseArea {
                        id: vizMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.visualizerStyle = (root.visualizerStyle === 0) ? 1 : 0
                    }

                    ToolTip.visible: vizMouse.containsMouse
                    ToolTip.text: "Toggle Visualizer Style (Fluid Waveform / Equalizer Bars)"
                }

                // Expanded Album View Shortcut
                Rectangle {
                    width: 26
                    height: 26
                    radius: Theme.cornerRadiusSmall
                    color: albBtnMouse.containsMouse ? Theme.selection : "transparent"
                    border.color: Theme.panelBorder
                    border.width: 1

                    VectorIcon {
                        anchors.centerIn: parent
                        name: "album_tracks"
                        width: 11
                        height: 11
                        color: Theme.textSecondary
                    }

                    MouseArea {
                        id: albBtnMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: switchToExpandedView(bridge.currentAlbum, bridge.currentArtist)
                    }

                    ToolTip.visible: albBtnMouse.containsMouse
                    ToolTip.text: "View Album in Expanded View"
                }

                // Toggle Play Queue Panel
                Rectangle {
                    width: 26
                    height: 26
                    radius: Theme.cornerRadiusSmall
                    color: (showRightPanel && rightPanelTab === 1) ? Theme.selection : (qBtnMouse.containsMouse ? Theme.surfaceElevated : "transparent")
                    border.color: (showRightPanel && rightPanelTab === 1) ? Theme.accent : Theme.panelBorder
                    border.width: 1

                    VectorIcon {
                        anchors.centerIn: parent
                        name: "queue"
                        width: 11
                        height: 11
                        color: (showRightPanel && rightPanelTab === 1) ? Theme.accent : Theme.textSecondary
                    }

                    MouseArea {
                        id: qBtnMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (showRightPanel && rightPanelTab === 1) {
                                showRightPanel = false;
                            } else {
                                rightPanelTab = 1;
                                showRightPanel = true;
                            }
                        }
                    }

                    ToolTip.visible: qBtnMouse.containsMouse
                    ToolTip.text: "Toggle Play Queue Side Panel"
                }
            }
        }

        // Main Stage Container (Expands to fill all remaining vertical and horizontal space)
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            // MODE A: Full-Stage Hero Presentation (When Lyrics are Toggled OFF)
            Item {
                anchors.fill: parent
                visible: !root.showLyrics

                ScrollView {
                    anchors.fill: parent
                    contentWidth: width
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                    ScrollBar.vertical.policy: ScrollBar.AsNeeded

                    ColumnLayout {
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: Math.min(parent.width - 48, 920)
                        spacing: Theme.spacingLarge

                        Item { height: Theme.spacingMedium; width: 1 }

                        // Upper Showcase: Large Artwork & Track Information Card
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: isWideLayout ? 320 : (artworkBox.Layout.preferredHeight + metaCol.implicitHeight + Theme.panelPadding * 3)
                            color: Theme.surface
                            radius: Theme.cornerRadiusMedium * 2
                            border.color: Theme.panelBorder
                            border.width: 1

                            GridLayout {
                                anchors.fill: parent
                                anchors.margins: Theme.panelPadding * 1.5
                                columns: isWideLayout ? 2 : 1
                                columnSpacing: Theme.spacingLarge * 1.5
                                rowSpacing: Theme.spacingLarge

                                // Showcase Album Artwork Box
                                Rectangle {
                                    id: artworkBox
                                    Layout.preferredWidth: isWideLayout ? Math.min(270, Math.min(parent.width * 0.42, 290)) : Math.min(220, Math.min(parent.width - 40, 220))
                                    Layout.preferredHeight: Layout.preferredWidth
                                    Layout.alignment: isWideLayout ? Qt.AlignVCenter : Qt.AlignHCenter
                                    radius: Theme.cornerRadiusMedium * 2
                                    color: Theme.surfaceElevated
                                    border.color: Theme.panelBorder
                                    border.width: 1
                                    clip: true

                                    Image {
                                        id: fullArtImage
                                        anchors.fill: parent
                                        source: bridge.currentArtUrl
                                        fillMode: Image.PreserveAspectCrop
                                        asynchronous: true
                                    }

                                    // Vinyl fallback graphic when no art is embedded
                                    Rectangle {
                                        anchors.fill: parent
                                        visible: fullArtImage.status !== Image.Ready || bridge.currentArtUrl.length === 0
                                        color: Theme.surfaceElevated

                                        ColumnLayout {
                                            anchors.centerIn: parent
                                            spacing: 10
                                            VectorIcon {
                                                Layout.alignment: Qt.AlignHCenter
                                                name: "disc"
                                                width: 68
                                                height: 68
                                                color: Theme.accent
                                            }
                                            Text {
                                                Layout.alignment: Qt.AlignHCenter
                                                text: bridge.currentAlbum.length > 0 ? bridge.currentAlbum : "PicAudio"
                                                font.pixelSize: Theme.fontSizeSmall
                                                font.bold: true
                                                color: Theme.textMuted
                                                elide: Text.ElideRight
                                                Layout.maximumWidth: artworkBox.width - 24
                                            }
                                        }
                                    }

                                    // Quick Play/Pause overlay on artwork hover
                                    Rectangle {
                                        anchors.fill: parent
                                        color: Qt.rgba(0, 0, 0, 0.4)
                                        opacity: artHoverMouse.containsMouse ? 1.0 : 0.0
                                        Behavior on opacity { NumberAnimation { duration: 150 } }

                                        Rectangle {
                                            anchors.centerIn: parent
                                            width: 52
                                            height: 52
                                            radius: 26
                                            color: Theme.accent

                                            VectorIcon {
                                                anchors.centerIn: parent
                                                name: bridge.isPlaying ? "pause" : "play"
                                                width: 20
                                                height: 20
                                                color: Theme.textPrimary
                                            }
                                        }

                                        MouseArea {
                                            id: artHoverMouse
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: bridge.togglePlayPause()
                                        }
                                    }
                                }

                                // Track Details & Audiophile Information
                                ColumnLayout {
                                    id: metaCol
                                    Layout.fillWidth: true
                                    Layout.fillHeight: isWideLayout
                                    Layout.alignment: isWideLayout ? Qt.AlignVCenter : Qt.AlignHCenter
                                    spacing: Theme.spacingMedium

                                    // Category / Context Breadcrumb
                                    RowLayout {
                                        spacing: 6
                                        Rectangle {
                                            width: 6
                                            height: 6
                                            radius: 3
                                            color: Theme.accent
                                        }
                                        Text {
                                            text: bridge.currentGenre.length > 0 ? (bridge.currentGenre.toUpperCase() + " • NOW STREAMING") : "MASTER AUDIO"
                                            font.pixelSize: Theme.fontSizeSmall - 1
                                            font.bold: true
                                            font.letterSpacing: 1.0
                                            color: Theme.textMuted
                                        }
                                    }

                                    // Track Title
                                    Text {
                                        Layout.fillWidth: true
                                        text: bridge.currentTrackTitle
                                        font.pixelSize: Theme.fontSizeTitle + 6
                                        font.bold: true
                                        color: Theme.textPrimary
                                        elide: Text.ElideRight
                                    }

                                    // Artist Name
                                    Text {
                                        Layout.fillWidth: true
                                        text: bridge.currentArtist
                                        font.pixelSize: Theme.fontSizeLarge + 3
                                        font.bold: true
                                        color: Theme.accentHover
                                        elide: Text.ElideRight
                                    }

                                    // Album & Release Year
                                    Text {
                                        Layout.fillWidth: true
                                        text: bridge.currentAlbum + (bridge.currentYear.length > 0 ? " (" + bridge.currentYear + ")" : "")
                                        font.pixelSize: Theme.fontSizeBase + 1
                                        color: Theme.textSecondary
                                        elide: Text.ElideRight
                                    }

                                    Item { height: 4; width: 1 }

                                    // Audiophile Quality Ribbon (Formal & Sleek)
                                    Flow {
                                        width: metaCol.width
                                        spacing: 6
                                        clip: true

                                        // Hi-Res Badge
                                        Rectangle {
                                            visible: bridge.currentSampleRate >= 48000 || bridge.currentBitDepth >= 24
                                            width: hiResTxt.implicitWidth + 12
                                            height: 22
                                            radius: Theme.cornerRadiusSmall
                                            color: Theme.selection
                                            border.color: Theme.accent
                                            border.width: 1
                                            Text {
                                                id: hiResTxt
                                                anchors.centerIn: parent
                                                text: "HI-RES AUDIO"
                                                font.pixelSize: Theme.fontSizeSmall - 1
                                                font.bold: true
                                                font.letterSpacing: 0.8
                                                color: Theme.accent
                                            }
                                        }

                                        // Codec
                                        Rectangle {
                                            visible: bridge.currentCodec.length > 0
                                            width: codecBadgeTxt.implicitWidth + 12
                                            height: 22
                                            radius: Theme.cornerRadiusSmall
                                            color: Theme.surfaceElevated
                                            border.color: Theme.panelBorder
                                            border.width: 1
                                            Text {
                                                id: codecBadgeTxt
                                                anchors.centerIn: parent
                                                text: bridge.currentCodec.toUpperCase()
                                                font.pixelSize: Theme.fontSizeSmall
                                                font.bold: true
                                                color: Theme.textPrimary
                                            }
                                        }

                                        // Bit Depth & Sample Rate
                                        Rectangle {
                                            visible: bridge.currentSampleRate > 0
                                            width: resBadgeTxt.implicitWidth + 12
                                            height: 22
                                            radius: Theme.cornerRadiusSmall
                                            color: Theme.surfaceElevated
                                            border.color: Theme.panelBorder
                                            border.width: 1
                                            Text {
                                                id: resBadgeTxt
                                                anchors.centerIn: parent
                                                text: (bridge.currentBitDepth > 0 ? (bridge.currentBitDepth + "b / ") : "") + (bridge.currentSampleRate / 1000.0).toFixed(1) + " kHz"
                                                font.pixelSize: Theme.fontSizeSmall
                                                color: Theme.textSecondary
                                            }
                                        }

                                        // Bitrate
                                        Rectangle {
                                            visible: bridge.currentBitrate > 0
                                            width: brBadgeTxt.implicitWidth + 12
                                            height: 22
                                            radius: Theme.cornerRadiusSmall
                                            color: Theme.surfaceElevated
                                            border.color: Theme.panelBorder
                                            border.width: 1
                                            Text {
                                                id: brBadgeTxt
                                                anchors.centerIn: parent
                                                text: bridge.currentBitrate + " kbps"
                                                font.pixelSize: Theme.fontSizeSmall
                                                color: Theme.textMuted
                                            }
                                        }

                                        // Bit-Perfect Direct Indicator
                                        Rectangle {
                                            width: bpTxt.implicitWidth + 12
                                            height: 22
                                            radius: Theme.cornerRadiusSmall
                                            color: Theme.surfaceElevated
                                            border.color: Theme.panelBorder
                                            border.width: 1
                                            RowLayout {
                                                anchors.centerIn: parent
                                                spacing: 4
                                                Rectangle { width: 4; height: 4; radius: 2; color: Theme.success }
                                                Text {
                                                    id: bpTxt
                                                    text: "BIT-PERFECT"
                                                    font.pixelSize: Theme.fontSizeSmall - 1
                                                    font.bold: true
                                                    color: Theme.textMuted
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // Middle Showcase: Expansive Wave Visualizer Card
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 120
                            color: Theme.surface
                            radius: Theme.cornerRadiusMedium * 2
                            border.color: Theme.panelBorder
                            border.width: 1

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: Theme.panelPadding * 1.2
                                spacing: 6

                                RowLayout {
                                    Layout.fillWidth: true
                                    Text {
                                        text: root.visualizerStyle === 0 ? "REAL-TIME FLUID HARMONIC WAVEFORM" : "REAL-TIME MULTI-BAND SPECTRUM EQUALIZER"
                                        font.pixelSize: Theme.fontSizeSmall - 1
                                        font.bold: true
                                        font.letterSpacing: 1.0
                                        color: Theme.textMuted
                                    }
                                    Item { Layout.fillWidth: true }
                                    Text {
                                        text: bridge.isPlaying ? "60 FPS • Real-Time Engine" : "Playback Suspended"
                                        font.pixelSize: Theme.fontSizeSmall - 1
                                        color: bridge.isPlaying ? Theme.accent : Theme.textMuted
                                    }
                                }

                                WaveVisualizer {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    active: bridge.isPlaying
                                    visualizerStyle: root.visualizerStyle
                                    backgroundColor: Theme.surfaceElevated
                                    visualizerRadius: Theme.cornerRadiusSmall
                                    visualizerBorderWidth: 1
                                }
                            }
                        }

                        // Lower Showcase: Scrub & Transport Deck Card
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 110
                            color: Theme.surface
                            radius: Theme.cornerRadiusMedium * 2
                            border.color: Theme.panelBorder
                            border.width: 1

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: Theme.panelPadding * 1.5
                                spacing: Theme.spacingMedium

                                // Seekbar & Timestamp Row
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 10

                                    Text {
                                        text: bridge.positionStr
                                        font.pixelSize: Theme.fontSizeSmall
                                        font.bold: true
                                        color: Theme.textSecondary
                                        Layout.preferredWidth: 38
                                    }

                                    // Smooth Interactive Scrub Slider
                                    Slider {
                                        id: heroSeekSlider
                                        Layout.fillWidth: true
                                        from: 0
                                        to: Math.max(1, bridge.durationMs)
                                        value: bridge.positionMs
                                        onMoved: bridge.seek(value)

                                        background: Rectangle {
                                            x: heroSeekSlider.leftPadding
                                            y: heroSeekSlider.topPadding + heroSeekSlider.availableHeight / 2 - height / 2
                                            implicitWidth: 200
                                            implicitHeight: 4
                                            width: heroSeekSlider.availableWidth
                                            height: implicitHeight
                                            radius: 2
                                            color: Theme.surfaceElevated

                                            Rectangle {
                                                width: heroSeekSlider.visualPosition * parent.width
                                                height: parent.height
                                                color: Theme.accent
                                                radius: 2
                                            }
                                        }

                                        handle: Rectangle {
                                            x: heroSeekSlider.leftPadding + heroSeekSlider.visualPosition * (heroSeekSlider.availableWidth - width)
                                            y: heroSeekSlider.topPadding + heroSeekSlider.availableHeight / 2 - height / 2
                                            implicitWidth: 12
                                            implicitHeight: 12
                                            radius: 6
                                            color: heroSeekSlider.pressed ? Theme.accentHover : Theme.accent
                                            border.color: Theme.textPrimary
                                            border.width: 1
                                        }
                                    }

                                    Text {
                                        text: bridge.showRemainingTime ? ("-" + bridge.durationStr) : bridge.durationStr
                                        font.pixelSize: Theme.fontSizeSmall
                                        color: Theme.textMuted
                                        Layout.preferredWidth: 42
                                        horizontalAlignment: Text.AlignRight

                                        MouseArea {
                                            anchors.fill: parent
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: bridge.showRemainingTime = !bridge.showRemainingTime
                                        }
                                    }
                                }

                                // Transport Action Buttons
                                RowLayout {
                                    Layout.alignment: Qt.AlignHCenter
                                    spacing: 16

                                    // Shuffle
                                    Rectangle {
                                        width: 32
                                        height: 32
                                        radius: 16
                                        color: bridge.shuffleMode !== 0 ? Theme.selection : "transparent"
                                        border.color: bridge.shuffleMode !== 0 ? Theme.accent : "transparent"
                                        VectorIcon {
                                            anchors.centerIn: parent
                                            name: "shuffle"
                                            width: 13
                                            height: 13
                                            color: bridge.shuffleMode !== 0 ? Theme.accent : Theme.textMuted
                                        }
                                        MouseArea {
                                            anchors.fill: parent
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: bridge.toggleShuffle()
                                        }
                                        ToolTip.text: "Shuffle: " + (bridge.shuffleMode !== 0 ? "On" : "Off")
                                    }

                                    // Previous
                                    Rectangle {
                                        width: 36
                                        height: 36
                                        radius: 18
                                        color: prevMouse.containsMouse ? Theme.surfaceElevated : "transparent"
                                        VectorIcon {
                                            anchors.centerIn: parent
                                            name: "previous"
                                            width: 15
                                            height: 15
                                            color: Theme.textPrimary
                                        }
                                        MouseArea {
                                            id: prevMouse
                                            anchors.fill: parent
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: bridge.previousTrack()
                                        }
                                        ToolTip.text: "Previous Track"
                                    }

                                    // Main Play / Pause Hero Button
                                    Rectangle {
                                        width: 48
                                        height: 48
                                        radius: 24
                                        color: playMasterMouse.containsMouse ? Theme.accentHover : Theme.accent

                                        VectorIcon {
                                            anchors.centerIn: parent
                                            name: bridge.isPlaying ? "pause" : "play"
                                            width: 18
                                            height: 18
                                            color: Theme.textPrimary
                                        }

                                        MouseArea {
                                            id: playMasterMouse
                                            anchors.fill: parent
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: bridge.togglePlayPause()
                                        }
                                        ToolTip.text: bridge.isPlaying ? "Pause" : "Play"
                                    }

                                    // Next
                                    Rectangle {
                                        width: 36
                                        height: 36
                                        radius: 18
                                        color: nextHeroMouse.containsMouse ? Theme.surfaceElevated : "transparent"
                                        VectorIcon {
                                            anchors.centerIn: parent
                                            name: "next"
                                            width: 15
                                            height: 15
                                            color: Theme.textPrimary
                                        }
                                        MouseArea {
                                            id: nextHeroMouse
                                            anchors.fill: parent
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: bridge.nextTrack()
                                        }
                                        ToolTip.text: "Next Track"
                                    }

                                    // Repeat
                                    Rectangle {
                                        width: 32
                                        height: 32
                                        radius: 16
                                        color: bridge.repeatMode !== 0 ? Theme.selection : "transparent"
                                        border.color: bridge.repeatMode !== 0 ? Theme.accent : "transparent"
                                        VectorIcon {
                                            anchors.centerIn: parent
                                            name: bridge.repeatMode === 2 ? "repeat_one" : "repeat"
                                            width: 13
                                            height: 13
                                            color: bridge.repeatMode !== 0 ? Theme.accent : Theme.textMuted
                                        }
                                        MouseArea {
                                            anchors.fill: parent
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: bridge.cycleRepeatMode()
                                        }
                                        ToolTip.text: "Repeat: " + (bridge.repeatMode === 0 ? "Off" : (bridge.repeatMode === 1 ? "All" : "One"))
                                    }
                                }
                            }
                        }

                        Item { height: Theme.spacingMedium; width: 1 }
                    }
                }
            }

            // MODE B: Balanced Studio & Lyrics Split Mode (When Lyrics are Toggled ON)
            Item {
                anchors.fill: parent
                visible: root.showLyrics

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: Theme.panelPadding * 1.5
                    spacing: Theme.spacingLarge

                    // Left Studio Column: Artwork, Track Info, Transport & Compact Wave
                    Rectangle {
                        Layout.fillHeight: true
                        Layout.preferredWidth: root.isWideLayout ? Math.min(460, Math.max(340, Math.round(parent.width * 0.44))) : -1
                        Layout.fillWidth: !root.isWideLayout
                        visible: root.isWideLayout || root.compactTab === 0
                        color: Theme.surface
                        radius: Theme.cornerRadiusMedium * 2
                        border.color: Theme.panelBorder
                        border.width: 1
                        clip: true

                        ScrollView {
                            anchors.fill: parent
                            anchors.margins: Theme.panelPadding * 1.2
                            contentWidth: width
                            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                            ScrollBar.vertical.policy: ScrollBar.AsNeeded

                            ColumnLayout {
                                anchors.horizontalCenter: parent.horizontalCenter
                                width: parent.width - 8
                                spacing: Theme.spacingMedium

                                Item { height: 4; width: 1 }

                                // Artwork
                                Rectangle {
                                    Layout.alignment: Qt.AlignHCenter
                                    Layout.preferredWidth: Math.min(220, Math.min(parent.width - 24, 240))
                                    Layout.preferredHeight: Layout.preferredWidth
                                    radius: Theme.cornerRadiusMedium * 2
                                    color: Theme.surfaceElevated
                                    border.color: Theme.panelBorder
                                    border.width: 1
                                    clip: true

                                    Image {
                                        anchors.fill: parent
                                        source: bridge.currentArtUrl
                                        fillMode: Image.PreserveAspectCrop
                                        asynchronous: true
                                    }

                                    ColumnLayout {
                                        anchors.centerIn: parent
                                        visible: bridge.currentArtUrl.length === 0
                                        spacing: 6
                                        VectorIcon { Layout.alignment: Qt.AlignHCenter; name: "disc"; width: 44; height: 44; color: Theme.accent }
                                        Text { Layout.alignment: Qt.AlignHCenter; text: bridge.currentAlbum; font.pixelSize: Theme.fontSizeSmall; color: Theme.textMuted }
                                    }
                                }

                                Item { height: 2; width: 1 }

                                // Song Title
                                Text {
                                    Layout.fillWidth: true
                                    text: bridge.currentTrackTitle
                                    font.pixelSize: Theme.fontSizeTitle + 2
                                    font.bold: true
                                    color: Theme.textPrimary
                                    horizontalAlignment: Text.AlignHCenter
                                    elide: Text.ElideRight
                                }

                                // Artist
                                Text {
                                    Layout.fillWidth: true
                                    text: bridge.currentArtist
                                    font.pixelSize: Theme.fontSizeLarge
                                    font.bold: true
                                    color: Theme.accentHover
                                    horizontalAlignment: Text.AlignHCenter
                                    elide: Text.ElideRight
                                }

                                // Album & Year
                                Text {
                                    Layout.fillWidth: true
                                    text: bridge.currentAlbum + (bridge.currentYear.length > 0 ? " (" + bridge.currentYear + ")" : "")
                                    font.pixelSize: Theme.fontSizeSmall
                                    color: Theme.textSecondary
                                    horizontalAlignment: Text.AlignHCenter
                                    elide: Text.ElideRight
                                }

                                // Audio specs pills
                                RowLayout {
                                    Layout.alignment: Qt.AlignHCenter
                                    spacing: 5

                                    Rectangle {
                                        visible: bridge.currentCodec.length > 0
                                        width: splitCodecTxt.implicitWidth + 8
                                        height: 18
                                        radius: Theme.cornerRadiusSmall
                                        color: Theme.surfaceElevated
                                        border.color: Theme.panelBorder
                                        Text {
                                            id: splitCodecTxt
                                            anchors.centerIn: parent
                                            text: bridge.currentCodec.toUpperCase()
                                            font.pixelSize: Theme.fontSizeSmall - 1
                                            font.bold: true
                                            color: Theme.accent
                                        }
                                    }

                                    Rectangle {
                                        visible: bridge.currentSampleRate > 0
                                        width: splitRateTxt.implicitWidth + 8
                                        height: 18
                                        radius: Theme.cornerRadiusSmall
                                        color: Theme.surfaceElevated
                                        border.color: Theme.panelBorder
                                        Text {
                                            id: splitRateTxt
                                            anchors.centerIn: parent
                                            text: (bridge.currentSampleRate / 1000.0).toFixed(1) + " kHz"
                                            font.pixelSize: Theme.fontSizeSmall - 1
                                            color: Theme.textSecondary
                                        }
                                    }

                                    Rectangle {
                                        visible: bridge.currentBitDepth > 0
                                        width: splitDepthTxt.implicitWidth + 8
                                        height: 18
                                        radius: Theme.cornerRadiusSmall
                                        color: Theme.surfaceElevated
                                        border.color: Theme.panelBorder
                                        Text {
                                            id: splitDepthTxt
                                            anchors.centerIn: parent
                                            text: bridge.currentBitDepth + "-bit"
                                            font.pixelSize: Theme.fontSizeSmall - 1
                                            color: Theme.textMuted
                                        }
                                    }
                                }

                                Item { height: 4; width: 1 }

                                // Compact Waveform Deck
                                WaveVisualizer {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 64
                                    active: bridge.isPlaying
                                    visualizerStyle: root.visualizerStyle
                                    backgroundColor: Theme.surfaceElevated
                                    visualizerRadius: Theme.cornerRadiusSmall
                                    visualizerBorderWidth: 1
                                }

                                Item { height: 4; width: 1 }

                                // Compact Transport Controls
                                RowLayout {
                                    Layout.alignment: Qt.AlignHCenter
                                    spacing: 12

                                    Rectangle {
                                        width: 28
                                        height: 28
                                        radius: 14
                                        color: "transparent"
                                        VectorIcon { anchors.centerIn: parent; name: "previous"; width: 12; height: 12; color: Theme.textPrimary }
                                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: bridge.previousTrack() }
                                    }

                                    Rectangle {
                                        width: 38
                                        height: 38
                                        radius: 19
                                        color: Theme.accent
                                        VectorIcon { anchors.centerIn: parent; name: bridge.isPlaying ? "pause" : "play"; width: 14; height: 14; color: Theme.textPrimary }
                                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: bridge.togglePlayPause() }
                                    }

                                    Rectangle {
                                        width: 28
                                        height: 28
                                        radius: 14
                                        color: "transparent"
                                        VectorIcon { anchors.centerIn: parent; name: "next"; width: 12; height: 12; color: Theme.textPrimary }
                                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: bridge.nextTrack() }
                                    }
                                }

                                Item { height: 8; width: 1 }
                            }
                        }
                    }

                    // Right Studio Column: Dedicated Synchronized Lyrics Stage
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        visible: root.isWideLayout || root.compactTab === 1
                        color: Theme.surface
                        radius: Theme.cornerRadiusMedium * 2
                        border.color: Theme.panelBorder
                        border.width: 1
                        clip: true

                        ColumnLayout {
                            anchors.fill: parent
                            spacing: 0

                            // Lyrics Header
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 38
                                color: Theme.surfaceElevated
                                border.color: Theme.panelBorder
                                border.width: 1

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: Theme.panelPadding * 1.2
                                    anchors.rightMargin: Theme.panelPadding * 1.2
                                    spacing: Theme.spacingMedium

                                    VectorIcon { name: "lyrics"; width: 12; height: 12; color: Theme.accent }

                                    Text {
                                        text: "LYRICS"
                                        font.pixelSize: Theme.fontSizeSmall
                                        font.bold: true
                                        font.letterSpacing: 1.0
                                        color: Theme.textPrimary
                                    }

                                    Rectangle {
                                        visible: bridge.hasLyrics
                                        width: syncTxt.implicitWidth + 10
                                        height: 18
                                        radius: Theme.cornerRadiusSmall
                                        color: bridge.isLyricsSynchronized ? Theme.selection : Theme.surface
                                        border.color: bridge.isLyricsSynchronized ? Theme.accent : Theme.panelBorder
                                        Text {
                                            id: syncTxt
                                            anchors.centerIn: parent
                                            text: bridge.isLyricsSynchronized ? "SYNCHRONIZED" : "PLAIN TEXT"
                                            font.pixelSize: Theme.fontSizeSmall - 1
                                            font.bold: true
                                            color: bridge.isLyricsSynchronized ? Theme.accent : Theme.textMuted
                                        }
                                    }

                                    Item { Layout.fillWidth: true }

                                    // Close Lyrics Button
                                    Rectangle {
                                        width: closeLrcRow.implicitWidth + 12
                                        height: 22
                                        radius: Theme.cornerRadiusSmall
                                        color: closeLrcMouse.containsMouse ? Theme.selection : "transparent"
                                        border.color: Theme.panelBorder

                                        RowLayout {
                                            id: closeLrcRow
                                            anchors.centerIn: parent
                                            spacing: 4
                                            VectorIcon { name: "clear"; width: 8; height: 8; color: Theme.textMuted }
                                            Text {
                                                text: "Hide Lyrics"
                                                font.pixelSize: Theme.fontSizeSmall - 1
                                                color: Theme.textSecondary
                                            }
                                        }

                                        MouseArea {
                                            id: closeLrcMouse
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: root.showLyrics = false
                                        }
                                        ToolTip.visible: closeLrcMouse.containsMouse
                                        ToolTip.text: "Close lyrics panel and return to full-stage presentation"
                                    }
                                }
                            }

                            // Lyrics Content Stage
                            Item {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                clip: true

                                // Empty Lyrics State
                                Rectangle {
                                    anchors.fill: parent
                                    color: "transparent"
                                    visible: !bridge.hasLyrics

                                    ColumnLayout {
                                        anchors.centerIn: parent
                                        spacing: Theme.spacingMedium
                                        width: Math.min(320, parent.width - 32)

                                        VectorIcon {
                                            Layout.alignment: Qt.AlignHCenter
                                            name: "lyrics"
                                            width: 42
                                            height: 42
                                            color: Theme.textMuted
                                        }

                                        Text {
                                            Layout.alignment: Qt.AlignHCenter
                                            text: "No Lyrics Found"
                                            font.pixelSize: Theme.fontSizeLarge
                                            font.bold: true
                                            color: Theme.textPrimary
                                        }

                                        Text {
                                            Layout.alignment: Qt.AlignHCenter
                                            Layout.fillWidth: true
                                            horizontalAlignment: Text.AlignHCenter
                                            wrapMode: Text.WordWrap
                                            text: "PicAudio looks for local .lrc/.txt files or embedded ID3v2/Vorbis tags alongside the audio track."
                                            font.pixelSize: Theme.fontSizeSmall
                                            color: Theme.textMuted
                                        }

                                        Item { height: 4; width: 1 }

                                        Rectangle {
                                            Layout.alignment: Qt.AlignHCenter
                                            width: reloadTxt.implicitWidth + 20
                                            height: 28
                                            radius: Theme.cornerRadiusSmall
                                            color: reloadMouse.containsMouse ? Theme.selection : Theme.surfaceElevated
                                            border.color: Theme.panelBorder

                                            RowLayout {
                                                anchors.centerIn: parent
                                                spacing: 6
                                                VectorIcon { name: "disc"; width: 10; height: 10; color: Theme.accent }
                                                Text {
                                                    id: reloadTxt
                                                    text: "Scan / Reload Lyrics"
                                                    font.pixelSize: Theme.fontSizeSmall
                                                    color: Theme.textSecondary
                                                }
                                            }

                                            MouseArea {
                                                id: reloadMouse
                                                anchors.fill: parent
                                                hoverEnabled: true
                                                cursorShape: Qt.PointingHandCursor
                                                onClicked: bridge.reloadLyrics()
                                            }
                                        }
                                    }
                                }

                                // Synchronized Karaoke Lyrics List
                                ListView {
                                    id: lyricsListView
                                    anchors.fill: parent
                                    anchors.margins: Theme.panelPadding * 1.5
                                    visible: bridge.hasLyrics
                                    model: bridge.currentLyricsLines
                                    clip: true
                                    boundsBehavior: Flickable.StopAtBounds
                                    highlightMoveDuration: 280
                                    preferredHighlightBegin: height * 0.35
                                    preferredHighlightEnd: height * 0.55
                                    highlightRangeMode: bridge.isLyricsSynchronized ? ListView.ApplyRange : ListView.NoHighlightRange

                                    delegate: Rectangle {
                                        id: lyricRow
                                        width: lyricsListView.width
                                        readonly property bool isCurrent: bridge.isLyricsSynchronized && (bridge.activeLyricIndex === index)
                                        readonly property bool isPast: bridge.isLyricsSynchronized && (bridge.activeLyricIndex > index)
                                        height: Math.max(34, lyricText.implicitHeight + (isCurrent ? 18 : 10))
                                        radius: Theme.cornerRadiusMedium
                                        color: isCurrent ? Theme.selection : (rowMouse.containsMouse ? Theme.surfaceElevated : "transparent")
                                        border.color: isCurrent ? Theme.accent : "transparent"
                                        border.width: isCurrent ? 1 : 0

                                        RowLayout {
                                            anchors.fill: parent
                                            anchors.leftMargin: 14
                                            anchors.rightMargin: 14
                                            spacing: 12

                                            // Timestamp pill
                                            Text {
                                                visible: bridge.isLyricsSynchronized && modelData.timeStr.length > 0
                                                text: modelData.timeStr
                                                font.pixelSize: Theme.fontSizeSmall - 1
                                                font.bold: lyricRow.isCurrent
                                                color: lyricRow.isCurrent ? Theme.accent : Theme.textMuted
                                                Layout.preferredWidth: 38
                                            }

                                            // Lyric line
                                            Text {
                                                id: lyricText
                                                text: modelData.text.length > 0 ? modelData.text : "♪"
                                                font.pixelSize: lyricRow.isCurrent ? (Theme.fontSizeLarge + 2) : Theme.fontSizeBase
                                                font.bold: lyricRow.isCurrent
                                                color: lyricRow.isCurrent ? Theme.textPrimary : (lyricRow.isPast ? Theme.textMuted : Theme.textSecondary)
                                                wrapMode: Text.WordWrap
                                                Layout.fillWidth: true
                                            }
                                        }

                                        MouseArea {
                                            id: rowMouse
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: {
                                                if (bridge.isLyricsSynchronized && modelData.timeMs >= 0) {
                                                    bridge.seekToLyric(index);
                                                }
                                            }
                                        }

                                        ToolTip.visible: rowMouse.containsMouse && bridge.isLyricsSynchronized
                                        ToolTip.text: "Click to jump playback to " + modelData.timeStr
                                    }

                                    ScrollBar.vertical: ScrollBar {
                                        active: true
                                        policy: ScrollBar.AsNeeded
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
