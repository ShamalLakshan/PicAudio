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
    anchors.centerIn: Overlay.overlay
    width: Math.min(640, Overlay.overlay ? Overlay.overlay.width - 32 : 640)
    height: Math.min(520, Overlay.overlay ? Overlay.overlay.height - 32 : 520)
    padding: 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property var diag: bridge.audioPipelineDiagnostics

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
        title: "Export Audio Diagnostics Report"
        fileMode: FileDialog.SaveFile
        nameFilters: ["Text Report (*.txt)", "All Files (*)"]
        onAccepted: {
            bridge.exportDiagnosticsReport(selectedFile.toString());
        }
    }

    contentItem: ColumnLayout {
        spacing: 0
        clip: true

        // Header
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            color: Theme.surfaceElevated
            border.color: Theme.panelBorder
            border.width: 1

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
                    text: "Audio Pipeline & Stream Diagnostics"
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
            Layout.preferredHeight: 44
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
                    Layout.preferredWidth: stateText.implicitWidth + 16
                    Layout.preferredHeight: 22
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
            ScrollBar.vertical.policy: ScrollBar.AsNeeded

            GridLayout {
                width: diagScrollView.availableWidth - 8
                columns: root.width < 500 ? 1 : 2
                columnSpacing: Theme.spacingMedium
                rowSpacing: Theme.spacingSmall

                component DiagRow: Rectangle {
                    property string label: ""
                    property string value: ""
                    Layout.fillWidth: true
                    Layout.preferredHeight: 38
                    radius: Theme.cornerRadiusSmall
                    color: Theme.background
                    border.color: Theme.panelBorder
                    border.width: 1

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
                    label: "Buffer Underruns (XRuns)"
                    value: diag ? (diag.xruns + "") : "0"
                }

                DiagRow {
                    label: "Playback Speed"
                    value: diag ? (diag.playbackRate + "x") : "1.0x"
                }

                DiagRow {
                    Layout.columnSpan: 2
                    label: "A-B Looping"
                    value: diag && diag.loopActive ? ("Active (" + (diag.loopPointA / 1000).toFixed(1) + "s -> " + (diag.loopPointB / 1000).toFixed(1) + "s)") : "Inactive"
                }
            }
        }

        // Footer Actions
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 46
            color: Theme.surfaceElevated
            border.color: Theme.panelBorder
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.panelPadding
                anchors.rightMargin: Theme.panelPadding
                spacing: Theme.spacingMedium

                Button {
                    text: "Export Report..."
                    implicitHeight: 28
                    onClicked: exportReportDialog.open()
                    contentItem: Text {
                        text: parent.text
                        font.pixelSize: Theme.fontSizeSmall
                        font.family: Theme.fontFamily
                        color: Theme.textPrimary
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: parent.hovered ? Theme.surfaceElevated : Theme.background
                        border.color: Theme.panelBorder
                        border.width: 1
                        radius: Theme.cornerRadiusSmall
                    }
                }

                Item { Layout.fillWidth: true }

                Button {
                    text: "Close"
                    implicitHeight: 28
                    implicitWidth: 80
                    onClicked: root.close()
                    contentItem: Text {
                        text: parent.text
                        font.pixelSize: Theme.fontSizeSmall
                        font.family: Theme.fontFamily
                        color: Theme.textPrimary
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: parent.hovered ? Theme.surfaceElevated : Theme.background
                        border.color: Theme.panelBorder
                        border.width: 1
                        radius: Theme.cornerRadiusSmall
                    }
                }
            }
        }
    }
}
