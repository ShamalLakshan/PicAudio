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
    y: Math.round(((Overlay.overlay ? Overlay.overlay.height : (parent ? parent.height : 400)) - height) / 2)
    width: Math.min(460, Overlay.overlay ? Overlay.overlay.width - 32 : 460)
    padding: 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    background: Rectangle {
        color: Theme.surface
        border.color: Theme.panelBorder
        border.width: 1
        radius: Theme.cornerRadiusMedium
    }

    onAboutToShow: {
        var ovW = Overlay.overlay ? Overlay.overlay.width : (parent ? parent.width : 500);
        var ovH = Overlay.overlay ? Overlay.overlay.height : (parent ? parent.height : 400);
        if (x <= 0 || y <= 0 || x > ovW - 60 || y > ovH - 60) {
            x = Math.max(16, Math.round((ovW - width) / 2));
            y = Math.max(16, Math.round((ovH - height) / 2));
        }

        urlInput.text = "";
        nameInput.text = "";
        urlInput.forceActiveFocus();
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
                    name: "external"
                    width: 14
                    height: 14
                    color: Theme.accent
                }

                Text {
                    text: "Open Network Audio Stream"
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

        // Body
        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: Theme.panelPadding
            spacing: Theme.spacingMedium

            Text {
                text: "Enter an HTTP or HTTPS stream URL to stream internet radio or remote audio files."
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.textMuted
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSmall

                Text {
                    text: "Stream URL"
                    font.pixelSize: Theme.fontSizeSmall
                    font.bold: true
                    color: Theme.textSecondary
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 32
                    color: Theme.background
                    border.color: urlInput.activeFocus ? Theme.accent : Theme.panelBorder
                    border.width: 1
                    radius: Theme.cornerRadiusSmall
                    clip: true

                    TextInput {
                        id: urlInput
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spacingMedium
                        anchors.rightMargin: Theme.spacingMedium
                        verticalAlignment: TextInput.AlignVCenter
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeBase
                        selectByMouse: true

                        Text {
                            text: "http://stream.radioparadise.com/flac"
                            color: Theme.textMuted
                            font.pixelSize: Theme.fontSizeBase
                            visible: !urlInput.text && !urlInput.activeFocus
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSmall

                Text {
                    text: "Station / Stream Name (Optional)"
                    font.pixelSize: Theme.fontSizeSmall
                    font.bold: true
                    color: Theme.textSecondary
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 32
                    color: Theme.background
                    border.color: nameInput.activeFocus ? Theme.accent : Theme.panelBorder
                    border.width: 1
                    radius: Theme.cornerRadiusSmall
                    clip: true

                    TextInput {
                        id: nameInput
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spacingMedium
                        anchors.rightMargin: Theme.spacingMedium
                        verticalAlignment: TextInput.AlignVCenter
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeBase
                        selectByMouse: true

                        Text {
                            text: "e.g. Radio Paradise FLAC"
                            color: Theme.textMuted
                            font.pixelSize: Theme.fontSizeBase
                            visible: !nameInput.text && !nameInput.activeFocus
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
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
                anchors.rightMargin: Theme.panelPadding
                spacing: Theme.spacingMedium

                Item { Layout.fillWidth: true }

                Rectangle {
                    Layout.preferredWidth: 80
                    Layout.preferredHeight: 30
                    radius: Theme.cornerRadiusSmall
                    color: cancelMouse.containsMouse ? Theme.surface : "transparent"
                    border.color: Theme.panelBorder
                    clip: true

                    Text {
                        anchors.centerIn: parent
                        text: "Cancel"
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.textSecondary
                    }

                    MouseArea {
                        id: cancelMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.close()
                    }
                }

                Rectangle {
                    Layout.preferredWidth: 90
                    Layout.preferredHeight: 30
                    radius: Theme.cornerRadiusSmall
                    color: enqueueMouse.containsMouse ? Theme.surface : Theme.surfaceElevated
                    border.color: Theme.panelBorder
                    clip: true

                    Text {
                        anchors.centerIn: parent
                        text: "Add to Queue"
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.textPrimary
                    }

                    MouseArea {
                        id: enqueueMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (urlInput.text.trim().length > 0) {
                                bridge.openNetworkStream(urlInput.text.trim(), nameInput.text.trim());
                                root.close();
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.preferredWidth: 90
                    Layout.preferredHeight: 30
                    radius: Theme.cornerRadiusSmall
                    color: playMouse.containsMouse ? Theme.accentHover : Theme.accent
                    clip: true

                    RowLayout {
                        anchors.centerIn: parent
                        spacing: Theme.spacingSmall
                        VectorIcon { name: "play"; width: 10; height: 10; color: Theme.textPrimary }
                        Text {
                            text: "Play Now"
                            font.pixelSize: Theme.fontSizeSmall
                            font.bold: true
                            color: Theme.textPrimary
                        }
                    }

                    MouseArea {
                        id: playMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (urlInput.text.trim().length > 0) {
                                bridge.openNetworkStream(urlInput.text.trim(), nameInput.text.trim());
                                root.close();
                            }
                        }
                    }
                }
            }
        }
    }

    ResizeGrip {
        target: root
        minWidth: 400
        minHeight: 240
    }
}
}
