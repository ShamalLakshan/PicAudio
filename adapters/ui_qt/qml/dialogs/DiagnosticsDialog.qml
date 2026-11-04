import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import ".."
import "../components"

Dialog {
    id: root
    modal: true
    focus: true
    x: Math.round(((Overlay.overlay ? Overlay.overlay.width : (parent ? parent.width : 740)) - width) / 2)
    y: Math.round(((Overlay.overlay ? Overlay.overlay.height : (parent ? parent.height : 580)) - height) / 2)
    width: Math.min(740, Overlay.overlay ? Math.max(600, Overlay.overlay.width - 48) : 740)
    height: Math.min(580, Overlay.overlay ? Math.max(480, Overlay.overlay.height - 48) : 580)
    padding: 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property var diag: bridge.audioPipelineDiagnostics

    onAboutToShow: {
        var ovW = Overlay.overlay ? Overlay.overlay.width : (parent ? parent.width : 740);
        var ovH = Overlay.overlay ? Overlay.overlay.height : (parent ? parent.height : 580);
        if (x <= 0 || y <= 0 || x > ovW - 60 || y > ovH - 60) {
            x = Math.max(16, Math.round((ovW - width) / 2));
            y = Math.max(16, Math.round((ovH - height) / 2));
        }
    }

    Timer {
        id: refreshTimer
        interval: 500
        running: root.visible
        repeat: true
        onTriggered: {
            root.diag = bridge.audioPipelineDiagnostics;
        }
    }

    background: Rectangle {
        color: Theme.surface
        border.color: Theme.panelBorder
        border.width: 1
        radius: Theme.cornerRadiusMedium
    }

    FileDialog {
        id: exportReportDialog
        title: "Export Audio Pipeline Diagnostics Report"
        fileMode: FileDialog.SaveFile
        nameFilters: ["Text Report (*.txt)", "All Files (*)"]
        onAccepted: {
            bridge.exportDiagnosticsReport(selectedFile.toString());
        }
    }

    contentItem: Item {
        clip: true

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // Header
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 44
                color: Theme.surfaceElevated
                border.color: Theme.panelBorder
                border.width: 1

                MovableHeaderArea {
                    target: root
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.panelPadding
                    anchors.rightMargin: Theme.panelPadding
                    spacing: Theme.spacingMedium

                    VectorIcon {
                        name: "equalizer"
                        width: 14
                        height: 14
                        color: Theme.accent
                    }

                    Text {
                        text: "Audio Pipeline Diagnostics"
                        font.pixelSize: Theme.fontSizeLarge
                        font.bold: true
                        color: Theme.textPrimary
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }

                    Rectangle {
                        width: 24
                        height: 24
                        radius: Theme.cornerRadiusSmall
                        color: closeMouse.containsMouse ? Theme.selection : "transparent"

                        VectorIcon {
                            anchors.centerIn: parent
                            name: "clear"
                            width: 10
                            height: 10
                            color: closeMouse.containsMouse ? Theme.textPrimary : Theme.textMuted
                        }

                        MouseArea {
                            id: closeMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.close()
                        }
                    }
                }
            }

            // Bit-Perfect Status Banner
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 48
                color: Theme.surfaceElevated
                border.color: Theme.panelBorder
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.panelPadding
                    anchors.rightMargin: Theme.panelPadding
                    spacing: Theme.spacingMedium

                    Rectangle {
                        width: 10
                        height: 10
                        radius: 5
                        color: diag && diag.isBitPerfect ? Theme.success : Theme.accent
                    }

                    ColumnLayout {
                        spacing: 2
                        Layout.fillWidth: true

                        Text {
                            text: diag && diag.isBitPerfect ? "Bit-Perfect Output Active" : "Shared Output Mode"
                            font.pixelSize: Theme.fontSizeSmall
                            font.bold: true
                            color: Theme.textPrimary
                        }

                        Text {
                            text: diag && diag.isBitPerfect ? "Integer stream • OS mixer bypassed" : "Standard output stream via system mixer"
                            font.pixelSize: 10
                            color: Theme.textMuted
                        }
                    }

                    Rectangle {
                        Layout.preferredWidth: 84
                        Layout.preferredHeight: 24
                        radius: Theme.cornerRadiusSmall
                        color: Theme.selection
                        border.color: Theme.accent
                        border.width: 1

                        Text {
                            id: stateText
                            anchors.centerIn: parent
                            text: diag ? (diag.state || "Stopped") : "Stopped"
                            font.pixelSize: 10
                            font.bold: true
                            color: Theme.accentHover
                        }
                    }
                }
            }

            // Diagnostics Properties List
            ScrollView {
                id: diagScrollView
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.margins: Theme.panelPadding
                clip: true
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                ScrollBar.vertical.policy: ScrollBar.AlwaysOn
                contentWidth: availableWidth

                GridLayout {
                    width: diagScrollView.availableWidth
                    columns: root.width < 500 ? 1 : 2
                    columnSpacing: Theme.spacingMedium
                    rowSpacing: Theme.spacingSmall

                    component DiagRow: Rectangle {
                        property string label: ""
                        property string value: ""
                        Layout.fillWidth: true
                        Layout.preferredWidth: (diagScrollView.availableWidth - (root.width < 500 ? 0 : Theme.spacingMedium)) / (root.width < 500 ? 1 : 2)
                        Layout.preferredHeight: 40
                        Layout.minimumHeight: 40
                        radius: Theme.cornerRadiusSmall
                        color: Theme.background
                        border.color: Theme.panelBorder
                        border.width: 1
                        clip: true

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Theme.spacingMedium
                            anchors.rightMargin: Theme.spacingMedium
                            spacing: Theme.spacingSmall

                            Text {
                                text: label
                                font.pixelSize: Theme.fontSizeSmall
                                color: Theme.textSecondary
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }

                            Text {
                                text: value
                                font.pixelSize: Theme.fontSizeSmall
                                font.bold: true
                                font.family: Theme.fontFamily
                                color: Theme.textPrimary
                                Layout.maximumWidth: parent.width * 0.58
                                horizontalAlignment: Text.AlignRight
                                elide: Text.ElideRight
                            }
                        }
                    }

                    DiagRow {
                        label: "Sampling Rate"
                        value: diag ? (diag.sampleRate + " Hz") : "44,100 Hz"
                    }

                    DiagRow {
                        label: "Bit Depth"
                        value: diag ? (diag.bitDepth + " bit") : "16 bit"
                    }

                    DiagRow {
                        label: "Audio Backend"
                        value: diag ? (diag.audioBackend || "ALSA / PipeWire") : "ALSA"
                    }

                    DiagRow {
                        label: "Output Device"
                        value: diag ? (diag.currentAudioDevice || "System Default") : "System Default"
                    }

                    DiagRow {
                        label: "Channel Configuration"
                        value: diag ? (diag.channels === 2 ? "Stereo (2 Ch)" : (diag.channels + " Ch")) : "Stereo (2 Ch)"
                    }

                    DiagRow {
                        label: "Audio Stream Codec"
                        value: diag ? (diag.codec || "FLAC / PCM") : "FLAC / PCM"
                    }

                    DiagRow {
                        label: "Stream Bitrate"
                        value: diag && diag.bitrate > 0 ? (diag.bitrate + " kbps") : "Lossless"
                    }

                    DiagRow {
                        label: "Hardware Buffer Latency"
                        value: diag ? (diag.bufferLatencyMs + " ms") : "50 ms"
                    }

                    DiagRow {
                        label: "Resampler"
                        value: diag ? (diag.resamplerQuality || "Bit-Exact (No Resampling)") : "Bit-Exact"
                    }

                    DiagRow {
                        label: "Dither / Quantization"
                        value: diag ? (diag.ditherMode || "None (Integer Passthrough)") : "None"
                    }

                    DiagRow {
                        label: "Active In-Process Decoder"
                        value: diag && diag.activeDecoder ? diag.activeDecoder : "Direct In-Process / FLAC"
                    }

                    DiagRow {
                        label: "Buffer Hardware Frames"
                        value: diag && diag.bufferFrames ? (diag.bufferFrames + " frames") : "2048 frames"
                    }

                    DiagRow {
                        label: "Buffer Underruns (XRuns)"
                        value: diag ? (diag.xruns + "") : "0"
                    }

                    DiagRow {
                        label: "Playback Speed"
                        value: diag ? (diag.playbackRate + "x") : "1.0x"
                    }

                    // Hardware Ring Buffer Fill Gauge Card
                    Rectangle {
                        Layout.columnSpan: root.width < 500 ? 1 : 2
                        Layout.fillWidth: true
                        Layout.preferredHeight: 52
                        Layout.minimumHeight: 52
                        radius: Theme.cornerRadiusSmall
                        color: Theme.background
                        border.color: Theme.panelBorder
                        border.width: 1
                        clip: true

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Theme.spacingMedium
                            anchors.rightMargin: Theme.spacingMedium
                            anchors.topMargin: Theme.spacingSmall
                            anchors.bottomMargin: Theme.spacingSmall
                            spacing: 4

                            RowLayout {
                                Layout.fillWidth: true
                                Text {
                                    text: "Hardware Ring Buffer Fill"
                                    font.pixelSize: Theme.fontSizeSmall
                                    color: Theme.textSecondary
                                    Layout.fillWidth: true
                                    elide: Text.ElideRight
                                }
                                Text {
                                    text: diag ? (diag.bufferFillPercentage.toFixed(1) + "%") : "0.0%"
                                    font.pixelSize: Theme.fontSizeSmall
                                    font.bold: true
                                    font.family: Theme.fontFamily
                                    color: (diag && diag.bufferFillPercentage < 15.0) ? Theme.danger : Theme.accent
                                    Layout.preferredWidth: 60
                                    horizontalAlignment: Text.AlignRight
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                height: 6
                                radius: 3
                                color: Theme.surfaceElevated
                                clip: true

                                Rectangle {
                                    width: Math.min(parent.width, Math.max(0, (diag ? diag.bufferFillPercentage : 0) / 100.0 * parent.width))
                                    height: parent.height
                                    radius: 3
                                    color: (diag && diag.bufferFillPercentage < 15.0) ? Theme.danger : Theme.accent
                                }
                            }
                        }
                    }

                    DiagRow {
                        Layout.columnSpan: root.width < 500 ? 1 : 2
                        Layout.preferredWidth: diagScrollView.availableWidth
                        label: "A-B Looping Segment"
                        value: diag && diag.loopActive ? ("Active (" + (diag.loopPointA / 1000).toFixed(1) + "s -> " + (diag.loopPointB / 1000).toFixed(1) + "s)") : "Inactive"
                    }
                }
            }

            // Footer Actions
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 50
                color: Theme.surfaceElevated
                border.color: Theme.panelBorder
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.panelPadding
                    anchors.rightMargin: 40
                    spacing: Theme.spacingMedium

                    Button {
                        text: "Export Report..."
                        implicitHeight: 32
                        implicitWidth: 130
                        padding: Theme.spacingSmall
                        leftPadding: Theme.spacingMedium
                        rightPadding: Theme.spacingMedium
                        hoverEnabled: true
                        onClicked: exportReportDialog.open()
                        contentItem: Text {
                            text: parent.text
                            font.pixelSize: Theme.fontSizeSmall
                            font.family: Theme.fontFamily
                            color: parent.hovered ? Theme.accentHover : Theme.textPrimary
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                        }
                        background: Rectangle {
                            color: parent.hovered ? Theme.surfaceElevated : Theme.background
                            border.color: parent.hovered ? Theme.accent : Theme.panelBorder
                            border.width: 1
                            radius: Theme.cornerRadiusSmall
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            acceptedButtons: Qt.NoButton
                        }
                    }

                    Item { Layout.fillWidth: true }

                    Button {
                        text: "Close"
                        implicitHeight: 32
                        implicitWidth: 90
                        padding: Theme.spacingSmall
                        leftPadding: Theme.spacingMedium
                        rightPadding: Theme.spacingMedium
                        hoverEnabled: true
                        onClicked: root.close()
                        contentItem: Text {
                            text: parent.text
                            font.pixelSize: Theme.fontSizeSmall
                            font.family: Theme.fontFamily
                            color: parent.hovered ? Theme.accentHover : Theme.textPrimary
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                        }
                        background: Rectangle {
                            color: parent.hovered ? Theme.surfaceElevated : Theme.background
                            border.color: parent.hovered ? Theme.accent : Theme.panelBorder
                            border.width: 1
                            radius: Theme.cornerRadiusSmall
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            acceptedButtons: Qt.NoButton
                        }
                    }
                }
            }
        }
    }

    ResizeGrip {
        target: root
        minWidth: 500
        minHeight: 380
    }
}
