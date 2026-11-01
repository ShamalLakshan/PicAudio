import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../components"

Dialog {
    id: root
    modal: true
    focus: true
    x: Math.round(((Overlay.overlay ? Overlay.overlay.width : (parent ? parent.width : 500)) - width) / 2)
    y: Math.round(((Overlay.overlay ? Overlay.overlay.height : (parent ? parent.height : 450)) - height) / 2)
    width: Math.min(480, Overlay.overlay ? Overlay.overlay.width - 32 : 480)
    height: Math.min(420, Overlay.overlay ? Overlay.overlay.height - 32 : 420)
    padding: 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    onAboutToShow: {
        var ovW = Overlay.overlay ? Overlay.overlay.width : (parent ? parent.width : 500);
        var ovH = Overlay.overlay ? Overlay.overlay.height : (parent ? parent.height : 450);
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
                    name: "music"
                    width: 14
                    height: 14
                    color: Theme.accent
                }

                Text {
                    text: "About PicAudio"
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

        // Body Content
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: Theme.panelPadding
            spacing: Theme.spacingMedium

            // App branding block
            RowLayout {
                spacing: Theme.spacingLarge
                Layout.alignment: Qt.AlignHCenter

                Rectangle {
                    width: 64
                    height: 64
                    radius: Theme.cornerRadiusMedium
                    color: Theme.surfaceElevated
                    border.color: Theme.panelBorder
                    border.width: 1
                    clip: true

                    Image {
                        anchors.centerIn: parent
                        source: "qrc:/icons/picaudio.png"
                        width: 52
                        height: 52
                        sourceSize.width: 52
                        sourceSize.height: 52
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                        mipmap: true
                    }
                }

                ColumnLayout {
                    spacing: 2

                    Text {
                        text: "PICAUDIO"
                        font.pixelSize: 18
                        font.bold: true
                        font.letterSpacing: 2.0
                        color: Theme.textPrimary
                    }

                    Text {
                        text: "Version 1.0.0-alpha"
                        font.pixelSize: Theme.fontSizeSmall
                        font.bold: true
                        color: Theme.accentHover
                    }

                    Text {
                        text: "Lightweight High-Fidelity Music Player"
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.textMuted
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Theme.panelBorder
            }

            // Specs table
            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSmall

                RowLayout {
                    Layout.fillWidth: true
                    Text { text: "Version:"; font.pixelSize: Theme.fontSizeSmall; color: Theme.textSecondary; Layout.preferredWidth: 120 }
                    Text { text: "0.1.0"; font.pixelSize: Theme.fontSizeSmall; font.bold: true; color: Theme.textPrimary; Layout.fillWidth: true; elide: Text.ElideRight }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Text { text: "Platform:"; font.pixelSize: Theme.fontSizeSmall; color: Theme.textSecondary; Layout.preferredWidth: 120 }
                    Text { text: "Linux (ALSA / PipeWire / PulseAudio)"; font.pixelSize: Theme.fontSizeSmall; font.bold: true; color: Theme.textPrimary; Layout.fillWidth: true; elide: Text.ElideRight }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Text { text: "License:"; font.pixelSize: Theme.fontSizeSmall; color: Theme.textSecondary; Layout.preferredWidth: 120 }
                    Text { text: "GNU AGPL v3"; font.pixelSize: Theme.fontSizeSmall; font.bold: true; color: Theme.textPrimary; Layout.fillWidth: true; elide: Text.ElideRight }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Text { text: "Repository:"; font.pixelSize: Theme.fontSizeSmall; color: Theme.textSecondary; Layout.preferredWidth: 120 }
                    Text {
                        text: "github.com/ShamalLakshan/PicAudio"
                        font.pixelSize: Theme.fontSizeSmall
                        font.underline: repoMouse.containsMouse
                        color: Theme.accent
                        Layout.fillWidth: true
                        elide: Text.ElideRight

                        MouseArea {
                            id: repoMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: Qt.openUrlExternally("https://github.com/ShamalLakshan/PicAudio")
                        }
                    }
                }
            }

            Item { Layout.fillHeight: true }
        }

        // Footer
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

    ResizeGrip {
        target: root
        minWidth: 400
        minHeight: 320
    }
}
}
