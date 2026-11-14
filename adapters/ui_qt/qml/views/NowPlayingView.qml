import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../components"

Item {
    id: root

    property int visualizerStyle: 0 // 0: Fluid Wave, 1: Audio Spectrum Bars
    property int rightPanelTab: 0 // 0: Up Next Queue, 1: Synchronized Lyrics
    readonly property bool hasActiveTrack: bridge.currentTrackTitle.length > 0
    readonly property bool isWideLayout: width >= 860

    // Auto-scroll lyrics when active index changes
    Connections {
        target: bridge
        function onActiveLyricIndexChanged() {
            if (root.rightPanelTab === 1 && bridge.activeLyricIndex >= 0 && lyricsListView.count > bridge.activeLyricIndex) {
                lyricsListView.positionViewAtIndex(bridge.activeLyricIndex, ListView.Center);
            }
        }
    }

    // Main 2-Column Layout
    RowLayout {
        anchors.fill: parent
        anchors.margins: Math.round(20 * Theme.uiScale)
        spacing: Math.round(20 * Theme.uiScale)

        // ==========================================
        // LEFT COLUMN: Primary Player & Stage Card
        // ==========================================
        Rectangle {
            id: leftCard
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.surface
            border.color: Theme.panelBorder
            border.width: 1
            radius: Math.round(8 * Theme.uiScale)
            clip: true

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Math.round(20 * Theme.uiScale)
                spacing: Math.round(14 * Theme.uiScale)

                // Top Area: Album Art + Track Info & Badges
                RowLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: Math.round(20 * Theme.uiScale)

                    // Album Artwork Card with vinyl fallback
                    Rectangle {
                        id: artFrame
                        Layout.alignment: Qt.AlignVCenter
                        Layout.preferredWidth: Math.min(Math.round(260 * Theme.uiScale), Math.round(leftCard.width * 0.4))
                        Layout.preferredHeight: Layout.preferredWidth
                        radius: Math.round(8 * Theme.uiScale)
                        color: Theme.background
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

                        // Vinyl fallback graphic when no art is available
                        Rectangle {
                            anchors.fill: parent
                            color: Theme.background
                            visible: fullArtImage.status !== Image.Ready

                            Rectangle {
                                anchors.centerIn: parent
                                width: Math.round(parent.width * 0.8)
                                height: width
                                radius: width / 2
                                color: "#181a1f"
                                border.color: "#2a2d35"
                                border.width: 2

                                Rectangle {
                                    anchors.centerIn: parent
                                    width: Math.round(parent.width * 0.65)
                                    height: width
                                    radius: width / 2
                                    color: "transparent"
                                    border.color: Qt.rgba(1, 1, 1, 0.08)
                                    border.width: 1
                                }

                                Rectangle {
                                    anchors.centerIn: parent
                                    width: Math.round(parent.width * 0.35)
                                    height: width
                                    radius: width / 2
                                    color: Theme.accent
                                    opacity: 0.9

                                    Rectangle {
                                        anchors.centerIn: parent
                                        width: Math.round(parent.width * 0.3)
                                        height: width
                                        radius: width / 2
                                        color: "#181a1f"
                                    }
                                }
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: bridge.togglePlayPause()
                        }
                    }

                    // Track Typography & Audio Format Badges
                    ColumnLayout {
                        id: metaCol
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignVCenter
                        spacing: Math.round(8 * Theme.uiScale)

                        // Context indicator
                        RowLayout {
                            spacing: 6
                            Rectangle {
                                width: 6
                                height: 6
                                radius: 3
                                color: bridge.isPlaying ? Theme.accent : Theme.textMuted
                            }
                            Text {
                                text: bridge.currentGenre.length > 0 ? (bridge.currentGenre.toUpperCase() + " • NOW PLAYING") : "NOW PLAYING"
                                font.pixelSize: Math.round(11 * Theme.uiScale)
                                font.bold: true
                                color: Theme.textMuted
                            }
                        }

                        // Track Title
                        Text {
                            Layout.fillWidth: true
                            text: bridge.currentTrackTitle.length > 0 ? bridge.currentTrackTitle : qsTr("No Track Playing")
                            font.pixelSize: Math.round(24 * Theme.uiScale)
                            font.bold: true
                            color: Theme.textPrimary
                            elide: Text.ElideRight
                        }

                        // Artist Name
                        Text {
                            Layout.fillWidth: true
                            text: bridge.currentArtist.length > 0 ? bridge.currentArtist : qsTr("PicAudio")
                            font.pixelSize: Math.round(16 * Theme.uiScale)
                            font.weight: Font.DemiBold
                            color: Theme.accentHover
                            elide: Text.ElideRight
                        }

                        // Album & Year
                        Text {
                            Layout.fillWidth: true
                            text: bridge.currentAlbum + (bridge.currentYear.length > 0 ? " (" + bridge.currentYear + ")" : "")
                            font.pixelSize: Theme.fontSizeSmall
                            color: Theme.textSecondary
                            elide: Text.ElideRight
                        }

                        Item { height: Math.round(4 * Theme.uiScale); width: 1 }

                        // Quality & Audiophile Badges Row
                        Flow {
                            Layout.fillWidth: true
                            spacing: Math.round(6 * Theme.uiScale)

                            // Hi-Res Audio Badge
                            Rectangle {
                                visible: bridge.currentSampleRate >= 48000 || bridge.currentBitDepth >= 24
                                width: hiResTxt.implicitWidth + 12
                                height: Math.round(22 * Theme.uiScale)
                                radius: Theme.cornerRadiusSmall
                                color: Theme.selection
                                border.color: Theme.accent
                                border.width: 1
                                Text {
                                    id: hiResTxt
                                    anchors.centerIn: parent
                                    text: "HI-RES AUDIO"
                                    font.pixelSize: Math.round(10 * Theme.uiScale)
                                    font.bold: true
                                    color: Theme.accent
                                }
                            }

                            // Codec Badge
                            Rectangle {
                                visible: bridge.currentCodec.length > 0
                                width: codecTxt.implicitWidth + 12
                                height: Math.round(22 * Theme.uiScale)
                                radius: Theme.cornerRadiusSmall
                                color: Theme.surfaceElevated
                                border.color: Theme.panelBorder
                                border.width: 1
                                Text {
                                    id: codecTxt
                                    anchors.centerIn: parent
                                    text: bridge.currentCodec.toUpperCase()
                                    font.pixelSize: Math.round(10 * Theme.uiScale)
                                    font.bold: true
                                    color: Theme.textPrimary
                                }
                            }

                            // Bit Depth & Sample Rate Badge
                            Rectangle {
                                visible: bridge.currentSampleRate > 0
                                width: resTxt.implicitWidth + 12
                                height: Math.round(22 * Theme.uiScale)
                                radius: Theme.cornerRadiusSmall
                                color: Theme.surfaceElevated
                                border.color: Theme.panelBorder
                                border.width: 1
                                Text {
                                    id: resTxt
                                    anchors.centerIn: parent
                                    text: (bridge.currentBitDepth > 0 ? (bridge.currentBitDepth + "b / ") : "") + (bridge.currentSampleRate / 1000.0).toFixed(1) + " kHz"
                                    font.pixelSize: Math.round(10 * Theme.uiScale)
                                    color: Theme.textSecondary
                                }
                            }

                            // Bitrate Badge
                            Rectangle {
                                visible: bridge.currentBitrate > 0
                                width: brTxt.implicitWidth + 12
                                height: Math.round(22 * Theme.uiScale)
                                radius: Theme.cornerRadiusSmall
                                color: Theme.surfaceElevated
                                border.color: Theme.panelBorder
                                border.width: 1
                                Text {
                                    id: brTxt
                                    anchors.centerIn: parent
                                    text: bridge.currentBitrate + " kbps"
                                    font.pixelSize: Math.round(10 * Theme.uiScale)
                                    color: Theme.textMuted
                                }
                            }

                            // Bit-Perfect Direct Indicator Badge
                            Rectangle {
                                width: bpTxt.implicitWidth + 14
                                height: Math.round(22 * Theme.uiScale)
                                radius: Theme.cornerRadiusSmall
                                color: Theme.surfaceElevated
                                border.color: Theme.panelBorder
                                border.width: 1

                                RowLayout {
                                    anchors.centerIn: parent
                                    spacing: 4
                                    Rectangle {
                                        width: 6
                                        height: 6
                                        radius: 3
                                        color: "#2ecc71"
                                    }
                                    Text {
                                        id: bpTxt
                                        text: "BIT-PERFECT"
                                        font.pixelSize: Math.round(10 * Theme.uiScale)
                                        font.bold: true
                                        color: Theme.textSecondary
                                    }
                                }
                            }
                        }
                    }
                }

                // Middle Area: WaveVisualizer
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.round(100 * Theme.uiScale)
                    color: Theme.surfaceElevated
                    radius: Theme.cornerRadiusSmall
                    border.color: Theme.panelBorder
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: Math.round(8 * Theme.uiScale)
                        spacing: Math.round(4 * Theme.uiScale)

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Text {
                                text: root.visualizerStyle === 0 ? qsTr("REAL-TIME FLUID HARMONIC WAVEFORM") : qsTr("REAL-TIME MULTI-BAND SPECTRUM EQUALIZER")
                                font.pixelSize: Math.round(10 * Theme.uiScale)
                                font.bold: true
                                color: Theme.textMuted
                            }

                            Item { Layout.fillWidth: true }

                            // Visualizer Style Switcher Button
                            Rectangle {
                                Layout.preferredWidth: vizStyleTxt.implicitWidth + 16
                                Layout.preferredHeight: Math.round(20 * Theme.uiScale)
                                radius: Math.round(3 * Theme.uiScale)
                                color: vizMouse.containsMouse ? Theme.selection : "transparent"
                                border.color: Theme.panelBorder
                                border.width: 1

                                Text {
                                    id: vizStyleTxt
                                    anchors.centerIn: parent
                                    text: root.visualizerStyle === 0 ? "Wave" : "Bars"
                                    font.pixelSize: Math.round(10 * Theme.uiScale)
                                    font.bold: true
                                    color: Theme.accent
                                }

                                MouseArea {
                                    id: vizMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: root.visualizerStyle = (root.visualizerStyle === 0) ? 1 : 0
                                }
                            }

                            Text {
                                text: bridge.isPlaying ? "60 FPS • Real-Time Engine" : "Playback Suspended"
                                font.pixelSize: Math.round(10 * Theme.uiScale)
                                color: bridge.isPlaying ? Theme.accent : Theme.textMuted
                            }
                        }

                        WaveVisualizer {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            active: bridge.isPlaying
                            visualizerStyle: root.visualizerStyle
                            backgroundColor: "transparent"
                            visualizerBorderWidth: 0
                        }
                    }
                }

                // Bottom Area: Seekbar Timeline & Transport Deck
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Math.round(6 * Theme.uiScale)

                    // Seekbar Row
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Math.round(10 * Theme.uiScale)

                        Text {
                            text: bridge.positionStr
                            font.pixelSize: Theme.fontSizeSmall
                            color: Theme.textSecondary
                        }

                        Slider {
                            id: trackSeekSlider
                            Layout.fillWidth: true
                            from: 0
                            to: Math.max(1, bridge.durationMs)
                            value: bridge.positionMs
                            hoverEnabled: true

                            onMoved: {
                                bridge.seek(value);
                            }
                        }

                        Text {
                            text: bridge.durationStr
                            font.pixelSize: Theme.fontSizeSmall
                            color: Theme.textSecondary
                        }
                    }

                    // Transport Controls Row
                    RowLayout {
                        Layout.alignment: Qt.AlignHCenter
                        spacing: Math.round(16 * Theme.uiScale)

                        // Shuffle Button
                        Rectangle {
                            Layout.preferredWidth: Math.round(38 * Theme.uiScale)
                            Layout.preferredHeight: Math.round(38 * Theme.uiScale)
                            radius: Math.round(4 * Theme.uiScale)
                            color: shufMouse.containsMouse ? Theme.surfaceElevated : "transparent"
                            border.color: bridge.shuffleMode ? Theme.accent : "transparent"
                            border.width: 1

                            VectorIcon {
                                anchors.centerIn: parent
                                name: "shuffle"
                                width: Math.round(16 * Theme.uiScale)
                                height: Math.round(16 * Theme.uiScale)
                                color: bridge.shuffleMode ? Theme.accent : Theme.textMuted
                            }

                            MouseArea {
                                id: shufMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: bridge.toggleShuffle()
                            }
                        }

                        // Previous Track Button
                        Rectangle {
                            Layout.preferredWidth: Math.round(44 * Theme.uiScale)
                            Layout.preferredHeight: Math.round(44 * Theme.uiScale)
                            radius: Math.round(6 * Theme.uiScale)
                            color: prevMouse.containsMouse ? Theme.surfaceElevated : Theme.background
                            border.color: Theme.panelBorder
                            border.width: 1

                            VectorIcon {
                                anchors.centerIn: parent
                                name: "previous"
                                width: Math.round(18 * Theme.uiScale)
                                height: Math.round(18 * Theme.uiScale)
                                color: Theme.textPrimary
                            }

                            MouseArea {
                                id: prevMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: bridge.previousTrack()
                            }
                        }

                        // Play / Pause Primary Button
                        Rectangle {
                            Layout.preferredWidth: Math.round(56 * Theme.uiScale)
                            Layout.preferredHeight: Math.round(56 * Theme.uiScale)
                            radius: width / 2
                            color: playMouse.containsMouse ? Theme.accentHover : Theme.accent

                            VectorIcon {
                                anchors.centerIn: parent
                                name: bridge.isPlaying ? "pause" : "play"
                                width: Math.round(22 * Theme.uiScale)
                                height: Math.round(22 * Theme.uiScale)
                                color: "#ffffff"
                            }

                            MouseArea {
                                id: playMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: bridge.togglePlayPause()
                            }
                        }

                        // Next Track Button
                        Rectangle {
                            Layout.preferredWidth: Math.round(44 * Theme.uiScale)
                            Layout.preferredHeight: Math.round(44 * Theme.uiScale)
                            radius: Math.round(6 * Theme.uiScale)
                            color: nextMouse.containsMouse ? Theme.surfaceElevated : Theme.background
                            border.color: Theme.panelBorder
                            border.width: 1

                            VectorIcon {
                                anchors.centerIn: parent
                                name: "next"
                                width: Math.round(18 * Theme.uiScale)
                                height: Math.round(18 * Theme.uiScale)
                                color: Theme.textPrimary
                            }

                            MouseArea {
                                id: nextMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: bridge.nextTrack()
                            }
                        }

                        // Repeat Button
                        Rectangle {
                            Layout.preferredWidth: Math.round(38 * Theme.uiScale)
                            Layout.preferredHeight: Math.round(38 * Theme.uiScale)
                            radius: Math.round(4 * Theme.uiScale)
                            color: repMouse.containsMouse ? Theme.surfaceElevated : "transparent"
                            border.color: bridge.repeatMode > 0 ? Theme.accent : "transparent"
                            border.width: 1

                            VectorIcon {
                                anchors.centerIn: parent
                                name: bridge.repeatMode === 2 ? "repeat_one" : "repeat"
                                width: Math.round(16 * Theme.uiScale)
                                height: Math.round(16 * Theme.uiScale)
                                color: bridge.repeatMode > 0 ? Theme.accent : Theme.textMuted
                            }

                            MouseArea {
                                id: repMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: bridge.cycleRepeatMode()
                            }
                        }
                    }
                }
            }
        }

        // ==========================================
        // RIGHT COLUMN: Up Next Queue & Lyrics Card
        // ==========================================
        Rectangle {
            id: rightCard
            Layout.preferredWidth: Math.min(Math.round(420 * Theme.uiScale), Math.round(root.width * 0.44))
            Layout.minimumWidth: Math.round(280 * Theme.uiScale)
            Layout.fillHeight: true
            color: Theme.surface
            border.color: Theme.panelBorder
            border.width: 1
            radius: Math.round(8 * Theme.uiScale)
            clip: true

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Math.round(16 * Theme.uiScale)
                spacing: Math.round(12 * Theme.uiScale)

                // Header with Segmented Switcher (Up Next vs Lyrics)
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Math.round(8 * Theme.uiScale)

                    // Tab: Up Next in Queue
                    Rectangle {
                        Layout.preferredHeight: Math.round(32 * Theme.uiScale)
                        Layout.preferredWidth: upNextTxt.implicitWidth + 24
                        radius: Theme.cornerRadiusSmall
                        color: root.rightPanelTab === 0 ? Theme.selection : (tab0Mouse.containsMouse ? Theme.surfaceElevated : "transparent")
                        border.color: root.rightPanelTab === 0 ? Theme.accent : Theme.panelBorder
                        border.width: 1

                        RowLayout {
                            id: upNextTxt
                            anchors.centerIn: parent
                            spacing: 6

                            VectorIcon {
                                name: "queue"
                                width: 12
                                height: 12
                                color: root.rightPanelTab === 0 ? Theme.accent : Theme.textSecondary
                            }

                            Text {
                                text: qsTr("Up Next") + (bridge.queueTrackModel && bridge.queueTrackModel.rowCount() > 0 ? (" (" + bridge.queueTrackModel.rowCount() + ")") : "")
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: root.rightPanelTab === 0
                                color: root.rightPanelTab === 0 ? Theme.accent : Theme.textSecondary
                            }
                        }

                        MouseArea {
                            id: tab0Mouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.rightPanelTab = 0
                        }
                    }

                    // Tab: Synchronized Lyrics
                    Rectangle {
                        Layout.preferredHeight: Math.round(32 * Theme.uiScale)
                        Layout.preferredWidth: lyricsTabTxt.implicitWidth + 24
                        radius: Theme.cornerRadiusSmall
                        color: root.rightPanelTab === 1 ? Theme.selection : (tab1Mouse.containsMouse ? Theme.surfaceElevated : "transparent")
                        border.color: root.rightPanelTab === 1 ? Theme.accent : Theme.panelBorder
                        border.width: 1

                        RowLayout {
                            id: lyricsTabTxt
                            anchors.centerIn: parent
                            spacing: 6

                            VectorIcon {
                                name: "lyrics"
                                width: 12
                                height: 12
                                color: root.rightPanelTab === 1 ? Theme.accent : (bridge.hasLyrics ? Theme.textPrimary : Theme.textMuted)
                            }

                            Text {
                                text: qsTr("Lyrics")
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: root.rightPanelTab === 1
                                color: root.rightPanelTab === 1 ? Theme.accent : Theme.textSecondary
                            }

                            // Dot indicator when lyrics exist
                            Rectangle {
                                visible: bridge.hasLyrics && root.rightPanelTab !== 1
                                width: 6
                                height: 6
                                radius: 3
                                color: Theme.accent
                            }
                        }

                        MouseArea {
                            id: tab1Mouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.rightPanelTab = 1
                        }
                    }

                    Item { Layout.fillWidth: true }

                    // Reload Lyrics button (when lyrics tab active)
                    Rectangle {
                        visible: root.rightPanelTab === 1
                        Layout.preferredWidth: Math.round(28 * Theme.uiScale)
                        Layout.preferredHeight: Math.round(28 * Theme.uiScale)
                        radius: Theme.cornerRadiusSmall
                        color: relMouse.containsMouse ? Theme.surfaceElevated : "transparent"
                        border.color: Theme.panelBorder
                        border.width: 1

                        Text {
                            anchors.centerIn: parent
                            text: "↻"
                            font.pixelSize: Math.round(14 * Theme.uiScale)
                            color: Theme.textSecondary
                        }

                        MouseArea {
                            id: relMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: bridge.reloadLyrics()
                        }

                        ToolTip.visible: relMouse.containsMouse
                        ToolTip.text: qsTr("Reload Lyrics")
                    }
                }

                // TAB 0: Up Next Queue Content
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    visible: root.rightPanelTab === 0
                    clip: true

                    ListView {
                        id: jukeQueue
                        anchors.fill: parent
                        clip: true
                        model: bridge.queueTrackModel
                        visible: bridge.queueTrackModel && bridge.queueTrackModel.rowCount() > 0
                        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                        spacing: Math.round(4 * Theme.uiScale)

                        delegate: Rectangle {
                            id: qItem
                            required property var model
                            required property int index
                            width: jukeQueue.width
                            height: Math.round(44 * Theme.uiScale)
                            color: qHover.hovered ? Qt.rgba(1, 1, 1, 0.05) : "transparent"
                            radius: Math.round(4 * Theme.uiScale)

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: Math.round(8 * Theme.uiScale)
                                anchors.rightMargin: Math.round(8 * Theme.uiScale)
                                spacing: Math.round(10 * Theme.uiScale)

                                Text {
                                    text: (qItem.index + 1)
                                    color: Theme.accent
                                    font.pixelSize: Theme.fontSizeSmall
                                    font.bold: true
                                    Layout.preferredWidth: Math.round(22 * Theme.uiScale)
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 2

                                    Text {
                                        text: qItem.model.title
                                        color: Theme.textPrimary
                                        font.pixelSize: Theme.fontSizeSmall
                                        font.weight: Font.DemiBold
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }

                                    Text {
                                        text: qItem.model.artist
                                        color: Theme.textSecondary
                                        font.pixelSize: Math.round(10 * Theme.uiScale)
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }
                                }

                                Text {
                                    text: qItem.model.durationStr || ""
                                    color: Theme.textSecondary
                                    font.pixelSize: Math.round(11 * Theme.uiScale)
                                }
                            }

                            HoverHandler { id: qHover; cursorShape: Qt.PointingHandCursor }
                            TapHandler {
                                onDoubleTapped: bridge.playQueueTrack(qItem.index)
                                onTapped: bridge.playQueueTrack(qItem.index)
                            }
                        }
                    }

                    // Empty Queue Placeholder
                    ColumnLayout {
                        anchors.centerIn: parent
                        visible: !bridge.queueTrackModel || bridge.queueTrackModel.rowCount() === 0
                        spacing: Math.round(8 * Theme.uiScale)

                        VectorIcon {
                            Layout.alignment: Qt.AlignHCenter
                            name: "queue"
                            width: Math.round(36 * Theme.uiScale)
                            height: Math.round(36 * Theme.uiScale)
                            color: Theme.textMuted
                        }

                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: qsTr("Queue is Empty")
                            font.pixelSize: Theme.fontSizeMedium
                            font.bold: true
                            color: Theme.textSecondary
                        }

                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: qsTr("Add songs from your library to queue them next")
                            font.pixelSize: Theme.fontSizeSmall
                            color: Theme.textMuted
                        }
                    }
                }

                // TAB 1: Synchronized Lyrics Content
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    visible: root.rightPanelTab === 1
                    clip: true

                    ListView {
                        id: lyricsListView
                        anchors.fill: parent
                        visible: bridge.hasLyrics
                        model: bridge.currentLyricsLines
                        clip: true
                        boundsBehavior: Flickable.StopAtBounds
                        preferredHighlightBegin: height * 0.35
                        preferredHighlightEnd: height * 0.55
                        highlightRangeMode: bridge.isLyricsSynchronized ? ListView.ApplyRange : ListView.NoHighlightRange
                        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                        spacing: Math.round(4 * Theme.uiScale)

                        delegate: Rectangle {
                            id: lyricItem
                            required property var modelData
                            required property int index
                            width: lyricsListView.width
                            readonly property bool isCurrent: bridge.isLyricsSynchronized && (bridge.activeLyricIndex === index)
                            height: Math.max(Math.round(32 * Theme.uiScale), lyricText.implicitHeight + (isCurrent ? 14 : 8))
                            radius: Theme.cornerRadiusSmall
                            color: isCurrent ? Theme.selection : (lRowMouse.containsMouse ? Theme.surfaceElevated : "transparent")
                            border.color: isCurrent ? Theme.accent : "transparent"
                            border.width: isCurrent ? 1 : 0

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: Math.round(10 * Theme.uiScale)
                                anchors.rightMargin: Math.round(10 * Theme.uiScale)
                                spacing: 8

                                Text {
                                    id: lyricText
                                    text: lyricItem.modelData.text || ""
                                    font.pixelSize: isCurrent ? (Theme.fontSizeBase + 1) : Theme.fontSizeSmall
                                    font.bold: isCurrent
                                    color: isCurrent ? Theme.accentHover : (lRowMouse.containsMouse ? Theme.textPrimary : Theme.textSecondary)
                                    wrapMode: Text.WordWrap
                                    Layout.fillWidth: true
                                    Layout.minimumWidth: 0
                                }
                            }

                            MouseArea {
                                id: lRowMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    if (lyricItem.modelData.timestampMs !== undefined) {
                                        bridge.seekToLyric(lyricItem.modelData.timestampMs);
                                    }
                                }
                            }
                        }
                    }

                    // Empty Lyrics Placeholder
                    ColumnLayout {
                        anchors.centerIn: parent
                        visible: !bridge.hasLyrics
                        spacing: Math.round(8 * Theme.uiScale)

                        VectorIcon {
                            Layout.alignment: Qt.AlignHCenter
                            name: "lyrics"
                            width: Math.round(36 * Theme.uiScale)
                            height: Math.round(36 * Theme.uiScale)
                            color: Theme.textMuted
                        }

                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: qsTr("No Synchronized Lyrics")
                            font.pixelSize: Theme.fontSizeMedium
                            font.bold: true
                            color: Theme.textSecondary
                        }

                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: qsTr("Embedded or sidecar (.lrc) lyrics not found for this track")
                            font.pixelSize: Theme.fontSizeSmall
                            color: Theme.textMuted
                        }
                    }
                }
            }
        }
    }
}
