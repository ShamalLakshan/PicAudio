import QtQuick
import QtQuick.Controls
import ".."

Item {
    id: gripRoot

    property var target: parent
    property real minWidth: 320
    property real minHeight: 200
    property real maxWidth: {
        if (target && target.parent && typeof target.parent.width !== "undefined") {
            return target.parent.width;
        }
        if (typeof Overlay !== "undefined" && Overlay.overlay) {
            return Overlay.overlay.width;
        }
        return 2400;
    }
    property real maxHeight: {
        if (target && target.parent && typeof target.parent.height !== "undefined") {
            return target.parent.height;
        }
        if (typeof Overlay !== "undefined" && Overlay.overlay) {
            return Overlay.overlay.height;
        }
        return 1800;
    }

    width: 20
    height: 20
    anchors.right: parent.right
    anchors.bottom: parent.bottom
    anchors.margins: 2
    z: 1000

    Canvas {
        id: gripCanvas
        anchors.fill: parent
        onPaint: {
            var ctx = getContext("2d");
            ctx.reset();
            ctx.clearRect(0, 0, width, height);
            ctx.strokeStyle = gripMouse.containsMouse ? Theme.accentHover : Theme.textMuted;
            ctx.lineWidth = 1.5;
            ctx.lineCap = "round";

            var offsets = [4, 8, 12];
            for (var i = 0; i < offsets.length; ++i) {
                var off = offsets[i];
                ctx.beginPath();
                ctx.moveTo(width - 3, height - off);
                ctx.lineTo(width - off, height - 3);
                ctx.stroke();
            }
        }
        Connections {
            target: Theme
            function onAccentChanged() { gripCanvas.requestPaint(); }
        }
    }

    MouseArea {
        id: gripMouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.SizeFDiagCursor
        property point lastGlobalPos: "0,0"

        onPressed: (mouse) => {
            lastGlobalPos = mapToGlobal(mouse.x, mouse.y);
        }

        onPositionChanged: (mouse) => {
            if (pressed && gripRoot.target) {
                var currentGlobal = mapToGlobal(mouse.x, mouse.y);
                var deltaX = currentGlobal.x - lastGlobalPos.x;
                var deltaY = currentGlobal.y - lastGlobalPos.y;
                lastGlobalPos = currentGlobal;

                var newW = gripRoot.target.width + deltaX;
                var newH = gripRoot.target.height + deltaY;
                gripRoot.target.width = Math.max(gripRoot.minWidth, Math.min(newW, gripRoot.maxWidth));
                gripRoot.target.height = Math.max(gripRoot.minHeight, Math.min(newH, gripRoot.maxHeight));
            }
        }

        onEntered: gripCanvas.requestPaint()
        onExited: gripCanvas.requestPaint()
    }
}
