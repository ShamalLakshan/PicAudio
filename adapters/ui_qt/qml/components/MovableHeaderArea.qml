import QtQuick
import QtQuick.Controls

MouseArea {
    id: dragArea
    property var target: null
    anchors.fill: parent
    z: 0
    cursorShape: pressed ? Qt.ClosedHandCursor : Qt.ArrowCursor
    property point lastGlobalPos: "0,0"

    onPressed: (mouse) => {
        lastGlobalPos = mapToGlobal(mouse.x, mouse.y);
    }

    onPositionChanged: (mouse) => {
        if (pressed && dragArea.target) {
            var currentGlobal = mapToGlobal(mouse.x, mouse.y);
            var deltaX = currentGlobal.x - lastGlobalPos.x;
            var deltaY = currentGlobal.y - lastGlobalPos.y;
            lastGlobalPos = currentGlobal;

            var newX = dragArea.target.x + deltaX;
            var newY = dragArea.target.y + deltaY;
            var overlayW = (typeof Overlay !== "undefined" && Overlay.overlay) ? Overlay.overlay.width : (dragArea.target.parent ? dragArea.target.parent.width : 1600);
            var overlayH = (typeof Overlay !== "undefined" && Overlay.overlay) ? Overlay.overlay.height : (dragArea.target.parent ? dragArea.target.parent.height : 1200);
            dragArea.target.x = Math.max(0, Math.min(newX, overlayW - dragArea.target.width));
            dragArea.target.y = Math.max(0, Math.min(newY, overlayH - dragArea.target.height));
        }
    }
}
