import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import ".."
import "../components"

Dialog {
    id: root
    modal: false
    focus: true
    title: "Audio Visualizer"

    x: Math.round(((Overlay.overlay ? Overlay.overlay.width : (parent ? parent.width : 840)) - width) / 2)
    y: Math.round(((Overlay.overlay ? Overlay.overlay.height : (parent ? parent.height : 520)) - height) / 2)
    width: Math.min(840, Overlay.overlay ? Math.max(680, Overlay.overlay.width - 48) : 840)
    height: Math.min(520, Overlay.overlay ? Math.max(380, Overlay.overlay.height - 48) : 520)
    padding: 0
    closePolicy: Popup.CloseOnEscape

    onAboutToShow: {
        var ovW = Overlay.overlay ? Overlay.overlay.width : (parent ? parent.width : 840);
        var ovH = Overlay.overlay ? Overlay.overlay.height : (parent ? parent.height : 520);
        if (x <= 0 || y <= 0 || x > ovW - 60 || y > ovH - 60) {
            x = Math.max(16, Math.round((ovW - width) / 2));
            y = Math.max(16, Math.round((ovH - height) / 2));
        }
    }

    background: Rectangle {
        color: Theme.surface
        border.color: Theme.panelBorder
        border.width: 1
        radius: Theme.cornerRadiusMedium
    }

    component StyledButton: Button {
        id: btnControl
        implicitHeight: 30
        leftPadding: Theme.spacingMedium
        rightPadding: Theme.spacingMedium
        topPadding: Theme.spacingSmall
        bottomPadding: Theme.spacingSmall
        hoverEnabled: true
        contentItem: Text {
            text: btnControl.text
            font.pixelSize: Theme.fontSizeSmall
            font.family: Theme.fontFamily
            font.bold: btnControl.highlighted
            color: btnControl.highlighted ? Theme.accentHover : Theme.textPrimary
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
        background: Rectangle {
            color: btnControl.highlighted ? Theme.selection : (btnControl.hovered ? Theme.surfaceElevated : Theme.background)
            border.color: btnControl.highlighted ? Theme.accent : (btnControl.hovered ? Theme.accentHover : Theme.panelBorder)
            border.width: 1
            radius: Theme.cornerRadiusSmall
        }
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            acceptedButtons: Qt.NoButton
        }
    }

    contentItem: Item {
        clip: true

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // Movable Header Bar
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.round(44 * Theme.uiScale)
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
                        width: 16
                        height: 16
                        color: Theme.accent
                    }

                    Text {
                        text: "Audio Visualizer"
                        font.bold: true
                        font.pixelSize: Theme.fontSizeBase
                        color: Theme.textPrimary
                    }

                    // Active Visualizer Plugin Badge
                    Rectangle {
                        Layout.preferredHeight: Math.round(22 * Theme.uiScale)
                        Layout.preferredWidth: vizNameText.implicitWidth + 14
                        radius: Theme.cornerRadiusSmall
                        color: Theme.selection
                        border.color: Theme.accent
                        border.width: 1

                        Text {
                            id: vizNameText
                            anchors.centerIn: parent
                            text: bridge.activeVisualizerName
                            font.pixelSize: Math.round(10 * Theme.uiScale)
                            font.bold: true
                            color: Theme.accent
                        }
                    }

                    Item { Layout.fillWidth: true }

                    // Style Switcher Segmented Control
                    Rectangle {
                        Layout.preferredHeight: Math.round(26 * Theme.uiScale)
                        Layout.preferredWidth: styleRow.implicitWidth + 8
                        radius: Theme.cornerRadiusSmall
                        color: Theme.background
                        border.color: Theme.panelBorder
                        border.width: 1

                        RowLayout {
                            id: styleRow
                            anchors.centerIn: parent
                            spacing: 4

                            Rectangle {
                                Layout.preferredHeight: Math.round(20 * Theme.uiScale)
                                Layout.preferredWidth: barsText.implicitWidth + 12
                                radius: Math.round(3 * Theme.uiScale)
                                color: bridge.visualizerStyle === 1 ? Theme.selection : "transparent"
                                border.color: bridge.visualizerStyle === 1 ? Theme.accent : "transparent"
                                border.width: 1

                                Text {
                                    id: barsText
                                    anchors.centerIn: parent
                                    text: "Bars / Spectrum"
                                    font.pixelSize: Math.round(10 * Theme.uiScale)
                                    font.bold: bridge.visualizerStyle === 1
                                    color: bridge.visualizerStyle === 1 ? Theme.accent : Theme.textSecondary
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: bridge.setVisualizerStyle(1)
                                }
                            }

                            Rectangle {
                                Layout.preferredHeight: Math.round(20 * Theme.uiScale)
                                Layout.preferredWidth: waveText.implicitWidth + 12
                                radius: Math.round(3 * Theme.uiScale)
                                color: bridge.visualizerStyle === 0 ? Theme.selection : "transparent"
                                border.color: bridge.visualizerStyle === 0 ? Theme.accent : "transparent"
                                border.width: 1

                                Text {
                                    id: waveText
                                    anchors.centerIn: parent
                                    text: "Fluid Wave"
                                    font.pixelSize: Math.round(10 * Theme.uiScale)
                                    font.bold: bridge.visualizerStyle === 0
                                    color: bridge.visualizerStyle === 0 ? Theme.accent : Theme.textSecondary
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: bridge.setVisualizerStyle(0)
                                }
                            }
                        }
                    }

                    // Window Close button
                    Rectangle {
                        Layout.preferredWidth: 26
                        Layout.preferredHeight: 26
                        radius: Theme.cornerRadiusSmall
                        color: closeMouse.containsMouse ? Theme.surface : "transparent"

                        VectorIcon {
                            anchors.centerIn: parent
                            name: "close"
                            width: 14
                            height: 14
                            color: closeMouse.containsMouse ? Theme.error : Theme.textMuted
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

            // Real-Time Visualizer Stage
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: Theme.background
                clip: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Theme.panelPadding
                    spacing: Theme.spacingSmall

                    WaveVisualizer {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        active: bridge.isPlaying
                        visualizerStyle: bridge.visualizerStyle
                        backgroundColor: "transparent"
                        visualizerBorderWidth: 0
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Theme.panelBorder
            }

            // Bottom Action & Info Bar
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.round(48 * Theme.uiScale)
                color: Theme.surfaceElevated

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.panelPadding
                    anchors.rightMargin: Math.round(44 * Theme.uiScale) // Safe margin for ResizeGrip
                    spacing: Theme.spacingMedium

                    // Transport mini indicator
                    Rectangle {
                        Layout.preferredWidth: 8
                        Layout.preferredHeight: 8
                        radius: 4
                        color: bridge.isPlaying ? "#2ecc71" : Theme.textMuted
                    }

                    Text {
                        Layout.fillWidth: true
                        text: bridge.currentTrackTitle.length > 0
                              ? (bridge.currentTrackTitle + " — " + bridge.currentArtist)
                              : qsTr("Playback Suspended")
                        font.pixelSize: Theme.fontSizeSmall
                        font.bold: bridge.isPlaying
                        color: bridge.isPlaying ? Theme.textPrimary : Theme.textMuted
                        elide: Text.ElideRight
                    }

                    Text {
                        text: bridge.isPlaying ? "60 FPS • Real-Time Engine" : "Idle"
                        font.pixelSize: Math.round(10 * Theme.uiScale)
                        color: bridge.isPlaying ? Theme.accent : Theme.textMuted
                    }

                    StyledButton {
                        text: "Now Playing Stage"
                        ToolTip.visible: hovered || activeFocus
                        ToolTip.text: "Switch main window to Now Playing stage"
                        onClicked: {
                            root.close();
                            bridge.openVisualizerView();
                        }
                    }

                    StyledButton {
                        text: "Close"
                        onClicked: root.close()
                    }
                }
            }
        }

        ResizeGrip {
            target: root
            minWidth: 560
            minHeight: 320
        }
    }
}
