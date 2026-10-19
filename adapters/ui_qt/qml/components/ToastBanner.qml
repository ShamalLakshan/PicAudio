import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

Item {
    id: root
    z: 9999
    anchors.top: parent ? parent.top : undefined
    anchors.horizontalCenter: parent ? parent.horizontalCenter : undefined
    anchors.topMargin: 48
    width: Math.min(520, parent ? parent.width - 32 : 520)
    height: toastCard.height
    visible: opacity > 0.01
    opacity: 0.0

    property string messageText: ""
    property string toastType: "info" // "info", "success", "warning", "error"
    property int durationMs: 4000

    function show(msg, type) {
        messageText = msg || "";
        toastType = type || "info";
        dismissTimer.stop();
        showAnimation.start();
        dismissTimer.restart();
    }

    function dismiss() {
        dismissTimer.stop();
        hideAnimation.start();
    }

    Connections {
        target: bridge
        function onToastRequested(msg, type) {
            root.show(msg, type);
        }
    }

    Timer {
        id: dismissTimer
        interval: root.durationMs
        repeat: false
        onTriggered: root.dismiss()
    }

    ParallelAnimation {
        id: showAnimation
        NumberAnimation {
            target: root
            property: "opacity"
            to: 1.0
            duration: 220
            easing.type: Easing.OutCubic
        }
        NumberAnimation {
            target: toastCard
            property: "y"
            from: -16
            to: 0
            duration: 220
            easing.type: Easing.OutCubic
        }
    }

    ParallelAnimation {
        id: hideAnimation
        NumberAnimation {
            target: root
            property: "opacity"
            to: 0.0
            duration: 200
            easing.type: Easing.InCubic
        }
        NumberAnimation {
            target: toastCard
            property: "y"
            to: -16
            duration: 200
            easing.type: Easing.InCubic
        }
    }

    Rectangle {
        id: toastCard
        width: parent.width
        implicitHeight: contentRow.implicitHeight + 20
        radius: Theme.cornerRadiusMedium
        color: Theme.surfaceElevated
        border.color: {
            if (root.toastType === "success") return Theme.accent;
            if (root.toastType === "error") return Theme.error;
            if (root.toastType === "warning") return Theme.warning;
            return Theme.accentHover;
        }
        border.width: 1
        clip: true

        // Left accent stripe indicator
        Rectangle {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 4
            color: {
                if (root.toastType === "success") return Theme.accent;
                if (root.toastType === "error") return Theme.error;
                if (root.toastType === "warning") return Theme.warning;
                return Theme.accentHover;
            }
        }

        RowLayout {
            id: contentRow
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 12
            anchors.topMargin: 10
            anchors.bottomMargin: 10
            spacing: Theme.spacingMedium

            VectorIcon {
                name: {
                    if (root.toastType === "success") return "check";
                    if (root.toastType === "error") return "clear";
                    if (root.toastType === "warning") return "info";
                    return "info";
                }
                width: 16
                height: 16
                color: {
                    if (root.toastType === "success") return Theme.accent;
                    if (root.toastType === "error") return Theme.error;
                    if (root.toastType === "warning") return Theme.warning;
                    return Theme.accentHover;
                }
                Layout.alignment: Qt.AlignVCenter
            }

            Text {
                text: root.messageText
                font.pixelSize: Theme.fontSizeBase
                font.family: Theme.fontFamily
                color: Theme.textPrimary
                wrapMode: Text.WordWrap
                elide: Text.ElideRight
                maximumLineCount: 2
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
            }

            Rectangle {
                width: 22
                height: 22
                radius: Theme.cornerRadiusSmall
                color: closeBtnMouse.containsMouse ? Theme.selection : "transparent"
                Layout.alignment: Qt.AlignVCenter

                VectorIcon {
                    anchors.centerIn: parent
                    name: "clear"
                    width: 10
                    height: 10
                    color: closeBtnMouse.containsMouse ? Theme.textPrimary : Theme.textMuted
                }

                MouseArea {
                    id: closeBtnMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.dismiss()
                }
            }
        }
    }
}
