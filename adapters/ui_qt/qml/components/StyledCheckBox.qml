import QtQuick
import QtQuick.Controls
import ".."

CheckBox {
    id: control
    implicitHeight: Math.max(28, (contentItem ? contentItem.implicitHeight : 0))
    hoverEnabled: true

    indicator: Rectangle {
        implicitWidth: 16
        implicitHeight: 16
        x: control.leftPadding
        y: Math.round(control.topPadding + (control.availableHeight - height) / 2)
        radius: Theme.cornerRadiusSmall
        color: control.checked ? Theme.accent : Theme.surfaceElevated
        border.color: control.checked ? Theme.accent : (control.hovered ? Theme.accentHover : Theme.panelBorder)
        border.width: 1

        VectorIcon {
            anchors.centerIn: parent
            width: 10
            height: 10
            name: "check"
            color: Theme.background
            visible: control.checked
        }
    }

    contentItem: Text {
        text: control.text
        font.pixelSize: Theme.fontSizeSmall
        font.family: Theme.fontFamily
        color: control.enabled ? (control.hovered ? Theme.textPrimary : Theme.textSecondary) : Theme.textMuted
        verticalAlignment: Text.AlignVCenter
        leftPadding: control.indicator ? (control.indicator.width + Theme.spacingMedium) : Theme.spacingMedium
        wrapMode: Text.WordWrap
        elide: Text.ElideRight
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        acceptedButtons: Qt.NoButton
    }
}
