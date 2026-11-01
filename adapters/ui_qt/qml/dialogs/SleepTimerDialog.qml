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
    width: Math.min(440, Overlay.overlay ? Overlay.overlay.width - 32 : 440)
    height: Math.min(380, Overlay.overlay ? Overlay.overlay.height - 32 : 380)
    padding: 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property int customMinutes: 30

    onAboutToShow: {
        var ovW = Overlay.overlay ? Overlay.overlay.width : (parent ? parent.width : 500);
        var ovH = Overlay.overlay ? Overlay.overlay.height : (parent ? parent.height : 450);
        if (x <= 0 || y <= 0 || x > ovW - 60 || y > ovH - 60) {
            x = Math.max(16, Math.round((ovW - width) / 2));
            y = Math.max(16, Math.round((ovH - height) / 2));
        }
    }

    function formatRemaining(totalSec) {
        if (totalSec <= 0) return "00:00";
        var hrs = Math.floor(totalSec / 3600);
        var mins = Math.floor((totalSec % 3600) / 60);
        var secs = totalSec % 60;
        var mStr = (mins < 10 ? "0" : "") + mins;
        var sStr = (secs < 10 ? "0" : "") + secs;
        if (hrs > 0) {
            return hrs + ":" + mStr + ":" + sStr;
        }
        return mStr + ":" + sStr;
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
                    name: "settings"
                    width: 14
                    height: 14
                    color: Theme.accent
                }

                Text {
                    text: "Playback Sleep Timer"
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

            // Active Countdown Display
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 90
                radius: Theme.cornerRadiusSmall
                color: bridge.isSleepTimerActive ? Theme.surfaceElevated : Theme.background
                border.color: bridge.isSleepTimerActive ? Theme.accent : Theme.panelBorder
                border.width: 1

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 4

                    Text {
                        text: bridge.isSleepTimerActive ? "TIMER RUNNING • STOPS PLAYBACK IN" : "SLEEP TIMER INACTIVE"
                        font.pixelSize: 10
                        font.bold: true
                        font.letterSpacing: 1.0
                        color: bridge.isSleepTimerActive ? Theme.accentHover : Theme.textMuted
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Text {
                        text: bridge.isSleepTimerActive ? root.formatRemaining(bridge.sleepTimerRemainingSec) : "--:--"
                        font.pixelSize: 28
                        font.bold: true
                        font.family: Theme.fontFamily
                        color: bridge.isSleepTimerActive ? Theme.textPrimary : Theme.textMuted
                        Layout.alignment: Qt.AlignHCenter
                    }
                }
            }

            // Quick Preset Buttons
            Text {
                text: "Select Duration"
                font.pixelSize: Theme.fontSizeSmall
                font.bold: true
                color: Theme.textSecondary
            }

            GridLayout {
                Layout.fillWidth: true
                columns: 3
                columnSpacing: Theme.spacingSmall
                rowSpacing: Theme.spacingSmall

                Repeater {
                    model: [15, 30, 45, 60, 90, 120]
                    delegate: Button {
                        Layout.fillWidth: true
                        implicitHeight: 32
                        text: modelData + " mins"
                        onClicked: {
                            root.customMinutes = modelData;
                            bridge.startSleepTimer(modelData);
                        }
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
                            border.color: (bridge.isSleepTimerActive && root.customMinutes === modelData) ? Theme.accent : Theme.panelBorder
                            border.width: 1
                            radius: Theme.cornerRadiusSmall
                        }
                    }
                }
            }

            // Custom minutes row
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingMedium

                Text {
                    text: "Custom:"
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.textSecondary
                }

                Rectangle {
                    Layout.preferredWidth: 80
                    Layout.preferredHeight: 32
                    color: Theme.background
                    border.color: Theme.panelBorder
                    border.width: 1
                    radius: Theme.cornerRadiusSmall

                    TextInput {
                        id: customMinutesInput
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        anchors.rightMargin: 8
                        verticalAlignment: TextInput.AlignVCenter
                        text: root.customMinutes.toString()
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeSmall
                        validator: IntValidator { bottom: 1; top: 1440 }
                        onTextChanged: {
                            var val = parseInt(text);
                            if (!isNaN(val) && val > 0) {
                                root.customMinutes = val;
                            }
                        }
                    }
                }

                Text {
                    text: "minutes"
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.textMuted
                }

                Item { Layout.fillWidth: true }

                Button {
                    text: "Start"
                    implicitHeight: 32
                    implicitWidth: 80
                    onClicked: {
                        bridge.startSleepTimer(root.customMinutes);
                    }
                    contentItem: Text {
                        text: parent.text
                        font.pixelSize: Theme.fontSizeSmall
                        font.bold: true
                        color: Theme.accentHover
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: parent.hovered ? Theme.selection : Theme.background
                        border.color: Theme.accent
                        border.width: 1
                        radius: Theme.cornerRadiusSmall
                    }
                }
            }

            Item { Layout.fillHeight: true }
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
                    text: "Cancel Sleep Timer"
                    enabled: bridge.isSleepTimerActive
                    implicitHeight: 28
                    onClicked: bridge.cancelSleepTimer()
                    contentItem: Text {
                        text: parent.text
                        font.pixelSize: Theme.fontSizeSmall
                        font.family: Theme.fontFamily
                        color: parent.enabled ? Theme.danger : Theme.textMuted
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: parent.hovered && parent.enabled ? Theme.surfaceElevated : Theme.background
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

    ResizeGrip {
        target: root
        minWidth: 380
        minHeight: 320
    }
}
}
